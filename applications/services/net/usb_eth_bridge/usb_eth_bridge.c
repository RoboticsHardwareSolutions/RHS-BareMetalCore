/**
 * usb_eth_bridge.c - Layer-2 bridge: USB CDC-Net <-> STM32F physical Ethernet.
 *
 * No own IP stack.  All Ethernet frames are forwarded transparently:
 *   USB host -> tud_network_recv_cb() -> ETH TX (mg_tcpip_driver_stm32f.tx)
 *   ETH RX (DMA IRQ -> recv_queue) -> bridge_worker drains -> USB TX (tud_network_xmit)
 *
 * To receive all frames from the LAN (not just those addressed to our MAC)
 * the ETH MAC filter is switched to promiscuous mode after driver init.
 *
 * This file defines the TinyUSB network callbacks (tud_network_recv_cb etc.)
 * and the USB descriptor data.  Do NOT link usb_cdc_net.c in the same build.
 *
 * USB descriptor block is derived from usb_cdc_net.c and kept in sync manually.
 * To avoid duplication long-term, consider extracting it to usb_cdc_net_desc.c/h.
 */

#include "usb_eth_bridge.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "mongoose.h"
#include "tusb.h"

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID                                                                    \
    (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) |           \
     _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) | _PID_MAP(ECM_RNDIS, 5) |          \
     _PID_MAP(NCM, 5))

enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };

#if CFG_TUD_NCM
#    define USB_BCD 0x0201
#else
#    define USB_BCD 0x0200
#endif

enum
{
#if CFG_TUD_ECM_RNDIS
    CONFIG_ID_ECM   = 0,
    CONFIG_ID_RNDIS = 1,
#else
    CONFIG_ID_NCM = 0,
#endif
    CONFIG_ID_COUNT
};

static tusb_desc_device_t const desc_bridge = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0101,
    .iManufacturer      = STRID_MANUFACTURER,
    .iProduct           = STRID_PRODUCT,
    .iSerialNumber      = STRID_SERIAL,
    .bNumConfigurations = CONFIG_ID_COUNT,
};

#define EPNUM_NET_NOTIF 0x81
#define EPNUM_NET_OUT   0x02
#define EPNUM_NET_IN    0x82

#if CFG_TUD_ECM_RNDIS

#    define MAIN_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_RNDIS_DESC_LEN)
#    define ALT_CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_ECM_DESC_LEN)

static uint8_t const rndis_fs_cfg[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL, 0, MAIN_CONFIG_TOTAL_LEN, 0, 100),
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, EPNUM_NET_NOTIF, 8, EPNUM_NET_OUT, EPNUM_NET_IN, 64),
};

static uint8_t const ecm_fs_cfg[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_ECM + 1, ITF_NUM_TOTAL, 0, ALT_CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_ECM_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, STRID_MAC, EPNUM_NET_NOTIF, 64,
                           EPNUM_NET_OUT, EPNUM_NET_IN, 64, CFG_TUD_NET_MTU),
};

static uint8_t const* const bridge_cfg_arr[CONFIG_ID_COUNT] = {
    [CONFIG_ID_RNDIS] = rndis_fs_cfg,
    [CONFIG_ID_ECM]   = ecm_fs_cfg,
};

#else  /* NCM */

#    define NCM_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_NCM_DESC_LEN)

static uint8_t const ncm_fs_cfg[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_NCM + 1, ITF_NUM_TOTAL, 0, NCM_CONFIG_TOTAL_LEN, 0, 100),
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE, STRID_MAC, EPNUM_NET_NOTIF, 64,
                           EPNUM_NET_OUT, EPNUM_NET_IN, 64, CFG_TUD_NET_MTU, 50,
                           (uint8_t)((uint8_t)NCM_NETWORK_CAPS_ETH_FILTER |
                                     (uint8_t)NCM_NETWORK_CAPS_NTB_INPUT_SIZE)),
};

static uint8_t const* const bridge_cfg_arr[CONFIG_ID_COUNT] = {
    [CONFIG_ID_NCM] = ncm_fs_cfg,
};

#endif  /* CFG_TUD_ECM_RNDIS */

/* String descriptors - same order/indices as usb_cdc_net.c */
uint8_t tud_network_mac_address[6] = {2, 2, 0x84, 0x6A, 0x96, 0};

static char const* bridge_string_arr[] = {
    [STRID_LANGID]       = (const char[]){0x09, 0x04},
    [STRID_MANUFACTURER] = "TinyUSB",
    [STRID_PRODUCT]      = "USB-ETH Bridge",
    [STRID_SERIAL]       = NULL,   /* filled from UID by usb_descriptors.c */
    [STRID_INTERFACE]    = "USB-ETH Bridge Network Interface",
    [STRID_MAC]          = NULL,   /* handled separately by TinyUSB */
};

static RHSHalUsbInterface bridge_usb_desc = {
    .device_desc           = &desc_bridge,
    .configuration_arr     = bridge_cfg_arr,
    .string_desc_arr       = (char const* const*)bridge_string_arr,
    .string_desc_arr_count = TU_ARRAY_SIZE(bridge_string_arr),
};

struct UsbEthBridge
{
    struct mg_tcpip_if                  eth_ifp;      /* ETH interface (for driver only) */
    struct mg_mgr                       eth_mgr;      /* minimal mgr - never polled for IP */
    struct mg_tcpip_driver_stm32f_data  eth_drv_data;
    RHSHalUsbInterface*                 prev_usb_intf;
    RHSThread*                          thread;
};

/* Single global instance - only one bridge may be active at a time */
static UsbEthBridge* g_bridge = NULL;

/* Called from TinyUSB task context when a frame arrives from the USB host. */
bool tud_network_recv_cb(const uint8_t* buf, uint16_t len)
{
    UsbEthBridge* b = g_bridge;
    if(b != NULL && b->eth_ifp.state >= MG_TCPIP_STATE_UP)
    {
        mg_tcpip_driver_stm32f.tx(buf, len, &b->eth_ifp);
    }
    tud_network_recv_renew();
    return true;
}

void tud_network_init_cb(void) {}

/* Called by TinyUSB to copy a queued TX frame into its internal buffer. */
uint16_t tud_network_xmit_cb(uint8_t* dst, void* ref, uint16_t arg)
{
    memcpy(dst, ref, arg);
    return arg;
}

static void bridge_eth_hw_init(void)
{
#if defined(STM32F407xx) || defined(STM32F765xx)
    uint16_t pins[] = {
        PIN('A', 1),  /* ETH_RMII_REF_CLK  */
        PIN('A', 2),  /* ETH_RMII_MDIO     */
        PIN('A', 7),  /* ETH_RMII_CRS_DV   */
        PIN('B', 11), /* ETH_RMII_TX_EN    */
        PIN('B', 12), /* ETH_RMII_TXD0     */
        PIN('B', 13), /* ETH_RMII_TXD1     */
        PIN('C', 1),  /* ETH_RMII_MDC      */
        PIN('C', 4),  /* ETH_RMII_RXD0     */
        PIN('C', 5),  /* ETH_RMII_RXD1     */
    };
    for(size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        gpio_init(pins[i], MG_GPIO_MODE_AF, MG_GPIO_OTYPE_PUSH_PULL,
                  MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 11);
    }
    NVIC_EnableIRQ(ETH_IRQn);
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;
    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN;
    RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;
    while((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) == 0) {}
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;
    while((RCC->AHB1RSTR & RCC_AHB1RSTR_ETHMACRST) != 0 ||
          (ETH->MACCR & 0x00008000) == 0) {}
#else
#    error "usb_eth_bridge: define Ethernet GPIO pins for your STM32 model"
#endif
}

/* =========================================================================
 * Bridge worker thread
 * ETH -> USB direction + link maintenance
 * ========================================================================= */

static int32_t bridge_worker(void* ctx)
{
    UsbEthBridge* b       = (UsbEthBridge*)ctx;
    uint64_t      last_1s = 0;

    for(;;)
    {
        uint64_t now     = mg_millis();
        bool     tick_1s = (now - last_1s) >= 1000U;
        if(tick_1s) last_1s = now;

        /* 1. Keep ETH link state machine alive (PHY polling, speed/duplex) */
        if(mg_tcpip_driver_stm32f.poll)
        {
            bool up = mg_tcpip_driver_stm32f.poll(&b->eth_ifp, tick_1s);
            /* Manually replicate the minimal link-up/down state change so that
             * tud_network_recv_cb can gate on ifp->state >= MG_TCPIP_STATE_UP. */
            if(tick_1s)
            {
                b->eth_ifp.state =
                    up ? MG_TCPIP_STATE_UP : MG_TCPIP_STATE_DOWN;
            }
        }

        char*  frame = NULL;
        size_t len   = 0;
        while((len = mg_queue_next(&b->eth_ifp.recv_queue, &frame)) > 0)
        {
            /* Spin until TinyUSB has buffer space (should be very brief) */
            while(!tud_network_can_xmit((uint16_t)len))
            {
                tud_task();
            }
            tud_network_xmit(frame, (uint16_t)len);
            mg_queue_del(&b->eth_ifp.recv_queue, len);
        }

        tud_task();

        rhs_delay_tick(0);
    }

    return 0;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

UsbEthBridge* usb_eth_bridge_start(const EthPhyConfig* phy_config)
{
    rhs_assert(g_bridge == NULL);  /* only one bridge instance allowed */

    UsbEthBridge* b = malloc(sizeof(UsbEthBridge));
    rhs_assert(b != NULL);
    memset(b, 0, sizeof(*b));

    /* --- Ethernet hardware + driver init ---------------------------------- */
    bridge_eth_hw_init();

    /* Bridge device MAC: use UID-derived locally administered address */
    uint8_t mac[6] = GENERATE_LOCALLY_ADMINISTERED_MAC(rhs_hal_version_uid());
    memcpy(b->eth_ifp.mac, mac, sizeof(mac));

    b->eth_drv_data.mdc_cr   = phy_config ? phy_config->mdc_cr   : MG_DRIVER_MDC_CR;
    b->eth_drv_data.phy_addr = phy_config ? phy_config->phy_addr : MG_TCPIP_PHY_ADDR;

    b->eth_ifp.driver      = &mg_tcpip_driver_stm32f;
    b->eth_ifp.driver_data = &b->eth_drv_data;
    /* No IP/mask/gw - we never run an IP stack on this interface.
     * mg_tcpip_init is called only to let the driver allocate DMA descriptors
     * and the recv_queue.  mg_mgr_poll is never called. */
    mg_mgr_init(&b->eth_mgr);
    mg_tcpip_init(&b->eth_mgr, &b->eth_ifp);

    /* Switch Ethernet MAC filter to promiscuous so ALL frames from the LAN
     * (not just those addressed to our device MAC) are received and can be
     * forwarded to the USB host. */
    ETH->MACFFR = MG_BIT(0);  /* PM = promiscuous mode */

    /* --- USB init --------------------------------------------------------- */
    b->prev_usb_intf = rhs_hal_usb_get_interface();
    rhs_hal_usb_set_interface(&bridge_usb_desc);
    rhs_hal_usb_reinit();
    tusb_init();

    /* --- Publish and start worker ----------------------------------------- */
    g_bridge = b;

    b->thread = rhs_thread_alloc("usb_eth_bridge", 2 * 1024, bridge_worker, b);
    rhs_thread_start(b->thread);

    MG_INFO(("USB-ETH bridge started, MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    return b;
}

void usb_eth_bridge_stop(UsbEthBridge* bridge)
{
    rhs_assert(bridge != NULL);
    rhs_crash("Not implemented yet");

    /* Sketch of teardown (fill in when implementing stop):
     *
     *   rhs_thread_join(bridge->thread);
     *   rhs_thread_free(bridge->thread);
     *
     *   tusb_deinit(0);
     *   rhs_hal_usb_set_interface(bridge->prev_usb_intf);
     *
     *   mg_tcpip_free(&bridge->eth_ifp);
     *   mg_mgr_free(&bridge->eth_mgr);
     *
     *   ethernet_deinit();   // disable IRQ, clocks, reset GPIO
     *
     *   g_bridge = NULL;
     *   free(bridge);
     */
}

__attribute__((weak)) bool mg_random(void* buf, size_t len)
{  // Use on-board RNG
    rhs_hal_random_fill_buf(buf, len);
    return true;
}
