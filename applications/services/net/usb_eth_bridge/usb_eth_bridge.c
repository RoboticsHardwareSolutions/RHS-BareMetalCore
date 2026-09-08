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
 * TinyUSB network callbacks are owned by rhs_hal_cdc_net.c.  This module
 * registers its handlers via rhs_hal_cdc_net_set() at start and clears them
 * at stop, allowing runtime switching with usb_cdc_net.
 */

#include "usb_eth_bridge.h"
#include "rhs.h"
#include "rhs_hal.h"
#include "mongoose.h"
#include "rhs_hal_cdc_net.h"
#include "cli.h"

struct UsbEthBridge
{
    struct mg_tcpip_if                 eth_ifp; /* ETH interface (for driver only) */
    struct mg_mgr                      eth_mgr; /* minimal mgr - never polled for IP */
    struct mg_tcpip_driver_stm32f_data eth_drv_data;
    RHSHalUsbInterface*                prev_usb_intf;
    RHSThread*                         thread;
    bool                               finish;
};

static TudNetOps bridge_ops = {0};

/* Called from TinyUSB task context when a frame arrives from the USB host. */
static bool bridge_recv_cb(const uint8_t* buf, uint16_t len, void* context)
{
    UsbEthBridge* b = (UsbEthBridge*) context;
    if (b != NULL && b->eth_ifp.state >= MG_TCPIP_STATE_UP)
    {
        mg_tcpip_driver_stm32f.tx(buf, len, &b->eth_ifp);
    }
    tud_network_recv_renew();
    return true;
}

static void bridge_init_cb(void* context)
{
    (void) context;
}

/* Called by TinyUSB to copy a queued TX frame into its internal buffer. */
static uint16_t bridge_xmit_cb(uint8_t* dst, void* ref, uint16_t arg, void* context)
{
    (void) context;
    memcpy(dst, ref, arg);
    return arg;
}

/* =========================================================================
 * Bridge worker thread
 * ETH -> USB direction + link maintenance
 * ========================================================================= */

static int32_t bridge_worker(void* ctx)
{
    UsbEthBridge* b       = (UsbEthBridge*) ctx;
    uint64_t      last_1s = 0;

    for (;;)
    {
        uint64_t now     = mg_millis();
        bool     tick_1s = (now - last_1s) >= 1000U;
        if (tick_1s)
            last_1s = now;

        /* 1. Keep ETH link state machine alive (PHY polling, speed/duplex) */
        if (mg_tcpip_driver_stm32f.poll)
        {
            bool up = mg_tcpip_driver_stm32f.poll(&b->eth_ifp, tick_1s);
            /* Manually replicate the minimal link-up/down state change so that
             * tud_network_recv_cb can gate on ifp->state >= MG_TCPIP_STATE_UP. */
            if (tick_1s)
            {
                b->eth_ifp.state = up ? MG_TCPIP_STATE_UP : MG_TCPIP_STATE_DOWN;
            }
        }

        char*  frame = NULL;
        size_t len   = 0;
        while ((len = mg_queue_next(&b->eth_ifp.recv_queue, &frame)) > 0)
        {
            /* Spin until TinyUSB has buffer space (should be very brief) */
            while (!tud_network_can_xmit((uint16_t) len))
            {
                tud_task();
            }
            tud_network_xmit(frame, (uint16_t) len);
            mg_queue_del(&b->eth_ifp.recv_queue, len);
        }

        tud_task();

        if (b->finish)
            break;

        rhs_delay_tick(0);
    }

    return 0;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

UsbEthBridge* usb_eth_bridge_start(const UsbEthBridgePhyConfig* phy_config)
{
    UsbEthBridge* b = malloc(sizeof(UsbEthBridge));
    rhs_assert(b != NULL);
    memset(b, 0, sizeof(*b));
    b->finish = false;

    /* --- Ethernet hardware + driver init ---------------------------------- */
    rhs_hal_eth_init();

    b->eth_drv_data.mdc_cr   = phy_config ? phy_config->mdc_cr : MG_DRIVER_MDC_CR;
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
    ETH->MACFFR = MG_BIT(0); /* PM = promiscuous mode */

    /* --- USB init --------------------------------------------------------- */
    // It is necessary that the mac for usb_cdc_net and usb_eth_bridge be different,
    // otherwise the system may incorrectly name the interface.
    tud_network_mac_address[5] = 0;

    b->prev_usb_intf = rhs_hal_usb_get_interface();
    rhs_hal_usb_set_interface(&usb_cdc_net_desc);
    rhs_hal_usb_reinit();
    tusb_init();

    /* --- Publish and start worker ----------------------------------------- */
    bridge_ops.recv    = bridge_recv_cb;
    bridge_ops.init    = bridge_init_cb;
    bridge_ops.xmit    = bridge_xmit_cb;
    bridge_ops.context = b;
    rhs_hal_cdc_net_set(&bridge_ops);

    b->thread = rhs_thread_alloc("usb_eth_bridge", 2 * 1024, bridge_worker, b);
    rhs_thread_start(b->thread);

    return b;
}

static void usb_eth_bridge_free(UsbEthBridge* bridge)
{
    rhs_thread_free(bridge->thread);
    mg_tcpip_free(&bridge->eth_ifp);
    mg_mgr_free(&bridge->eth_mgr);
    free(bridge);
}

void usb_eth_bridge_stop(UsbEthBridge* bridge)
{
    rhs_assert(bridge != NULL);
    rhs_hal_cdc_net_clear();
    bridge->finish = true;
    rhs_thread_join(bridge->thread);
    usb_eth_bridge_free(bridge);
    tusb_deinit(0);
    rhs_hal_usb_set_interface(bridge->prev_usb_intf);
    rhs_hal_eth_deinit();  // disable IRQ, clocks, reset GPIO
}

static void usb_eth_bridge_cli(char* args, void* context)
{
    if (args == NULL)
    {
        printf("usb_cdc_app command received. Usage:\r\n");
        printf("  usb_eth_bridge_app start - Start USB CDC network interface\r\n");
        printf("  usb_eth_bridge_app stop  - Stop USB CDC network interface\r\n");
    }
    else if (strstr(args, "start") == args)
    {
        usb_eth_bridge_start(NULL);
        printf("usb_eth_bridge_app command received with args: %s\r\n", args);
    }
    else if (strstr(args, "stop") == args)
    {
        usb_eth_bridge_stop(NULL);
        printf("USB CDC network interface stopped\r\n");
    }
}

void usb_eth_bridge_start_up(void)
{
    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "usb_eth_bridge_app", usb_eth_bridge_cli, NULL);
    rhs_record_close(RECORD_CLI);
}
