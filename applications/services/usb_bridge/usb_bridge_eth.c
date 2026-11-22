/* usb_bridge Eth target: L2 frames over USB NCM, the Ethernet MAC through the mongoose STM32F driver.
 * No IP stack on the device. The USB interface is switched to NCM and TinyUSB runs in the bridge thread (poll()).
 *
 *   USB host -> tud_network_recv_cb() -> ETH TX (mg_tcpip_driver_stm32f.tx)
 *   ETH RX (DMA IRQ -> recv_queue) -> poll() -> USB TX (tud_network_xmit)
 *
 * The MAC is promiscuous so all LAN frames reach the host. Only on boards with RHS_HAL_ETH. */
#include "usb_bridge_i.h"

#if defined(RHS_HAL_ETH)

#    include "mongoose.h"
#    include "rhs_hal_cdc_net.h"

#    define TAG "usb_bridge_eth"

#    define ETH_USB_TX_TIMEOUT_MS 100

typedef struct
{
    struct mg_tcpip_if                 eth_ifp; /**< ETH interface, for the driver only */
    struct mg_mgr                      eth_mgr; /**< Minimal mgr, never polled for IP */
    struct mg_tcpip_driver_stm32f_data eth_drv_data;
    bool                               eth_running;
    uint64_t                           last_1s;
    RHSHalUsbInterface*                prev_usb_intf;
    TudNetOps                          ops;
} UsbBridgeEth;

static UsbBridgeEth* eth_ctx(UsbBridge* bridge)
{
    return (UsbBridgeEth*) bridge->backend_ctx;
}

static void eth_send(UsbBridge* bridge, const uint8_t* frame, size_t len)
{
    UsbBridgeEth* eth = eth_ctx(bridge);
    if (eth->eth_running && eth->eth_ifp.state >= MG_TCPIP_STATE_UP)
    {
        mg_tcpip_driver_stm32f.tx(frame, len, &eth->eth_ifp);
        bridge->st.device_tx++;
    }
    else
    {
        bridge->st.device_tx_errors++;
    }
}

static void usb_send(UsbBridge* bridge, const uint8_t* frame, size_t len)
{
    if (!tud_ready())
    {
        bridge->st.usb_tx_drops++;
        return;
    }
    uint32_t deadline = rhs_get_tick() + ETH_USB_TX_TIMEOUT_MS;
    while (!tud_network_can_xmit((uint16_t) len))
    {
        if ((int32_t) (deadline - rhs_get_tick()) <= 0)
        {
            bridge->st.usb_tx_drops++;
            return;
        }
        tud_task();
    }
    tud_network_xmit((void*) frame, (uint16_t) len);
    bridge->st.usb_tx++;
}

/* TinyUSB network callbacks, called from tud_task() in the bridge thread */

static bool eth_usb_recv_cb(const uint8_t* buf, uint16_t len, void* context)
{
    UsbBridge* bridge = (UsbBridge*) context;
    bridge->st.usb_rx++;
    if (usb_bridge_has_device(bridge))
    {
        eth_send(bridge, buf, len);
    }
    usb_bridge_app_deliver_data(bridge, UsbBridgePortUsb, buf, len);
    tud_network_recv_renew();
    return true;
}

static void eth_usb_init_cb(void* context)
{
    UNUSED(context);
}

/* Copy a frame queued with tud_network_xmit() into the TinyUSB buffer */
static uint16_t eth_usb_xmit_cb(uint8_t* dst, void* ref, uint16_t arg, void* context)
{
    UNUSED(context);
    memcpy(dst, ref, arg);
    return arg;
}

static void eth_start(UsbBridge* bridge)
{
    UsbBridgeEth* eth = eth_ctx(bridge);

    rhs_hal_eth_init();
    eth->eth_drv_data.mdc_cr   = MG_DRIVER_MDC_CR;
    eth->eth_drv_data.phy_addr = MG_TCPIP_PHY_ADDR;
    eth->eth_ifp.driver        = &mg_tcpip_driver_stm32f;
    eth->eth_ifp.driver_data   = &eth->eth_drv_data;

    // No IP/mask/gw: mg_tcpip_init only lets the driver allocate DMA descriptors and the recv_queue,
    // mg_mgr_poll is never called
    mg_mgr_init(&eth->eth_mgr);
    mg_log_set(MG_LL_ERROR);
    mg_tcpip_init(&eth->eth_mgr, &eth->eth_ifp);

    // Promiscuous MAC filter: all LAN frames, not only the ones for our MAC, go to the host
    ETH->MACFFR = MG_BIT(0);
    eth->eth_running = true;
}

static void eth_stop(UsbBridge* bridge)
{
    UsbBridgeEth* eth = eth_ctx(bridge);
    if (!eth->eth_running)
    {
        return;
    }
    mg_tcpip_free(&eth->eth_ifp);
    mg_mgr_free(&eth->eth_mgr);
    rhs_hal_eth_deinit();  // disable IRQ, clocks, reset GPIO
    eth->eth_running = false;
}

/*********************************** BACKEND ************************************/

static const char* eth_unsupported(UsbBridgeMode mode)
{
    UNUSED(mode);
    return NULL;
}

static void eth_backend_start(UsbBridge* bridge)
{
    UsbBridgeEth* eth   = calloc(1, sizeof(UsbBridgeEth));
    bridge->backend_ctx = eth;

    if (usb_bridge_has_device(bridge))
    {
        eth_start(bridge);
    }

    eth->prev_usb_intf = rhs_hal_usb_get_interface();
    rhs_hal_usb_set_interface(&usb_cdc_enet_desc, NULL);

    // usb_cdc_net and the bridge need different MACs, otherwise the host may name the interface wrong
    tud_network_mac_address[5] = 0;

    rhs_hal_usb_reinit();
    tusb_init();

    eth->ops.recv    = eth_usb_recv_cb;
    eth->ops.init    = eth_usb_init_cb;
    eth->ops.xmit    = eth_usb_xmit_cb;
    eth->ops.context = bridge;
    rhs_hal_cdc_net_set(&eth->ops);
}

static void eth_backend_stop(UsbBridge* bridge)
{
    UsbBridgeEth* eth = eth_ctx(bridge);

    rhs_hal_cdc_net_clear();
    tusb_deinit(0);
    rhs_hal_usb_set_interface(eth->prev_usb_intf, eth->prev_usb_intf ? eth->prev_usb_intf->context : NULL);
    eth_stop(bridge);

    free(eth);
    bridge->backend_ctx = NULL;
}

static void eth_on_events(UsbBridge* bridge, uint32_t events)
{
    // Everything happens in poll()
    UNUSED(bridge);
    UNUSED(events);
}

static void eth_poll(UsbBridge* bridge)
{
    UsbBridgeEth* eth = eth_ctx(bridge);

    if (eth->eth_running)
    {
        uint64_t now     = mg_millis();
        bool     tick_1s = (now - eth->last_1s) >= 1000U;
        if (tick_1s)
        {
            eth->last_1s = now;
        }

        // Keep the ETH link state machine alive (PHY polling, speed/duplex)
        if (mg_tcpip_driver_stm32f.poll)
        {
            bool up = mg_tcpip_driver_stm32f.poll(&eth->eth_ifp, tick_1s);
            if (tick_1s)
            {
                eth->eth_ifp.state = up ? MG_TCPIP_STATE_UP : MG_TCPIP_STATE_DOWN;
            }
        }

        char*  frame = NULL;
        size_t len   = 0;
        while ((len = mg_queue_next(&eth->eth_ifp.recv_queue, &frame)) > 0)
        {
            bridge->st.device_rx++;
            usb_send(bridge, (const uint8_t*) frame, len);
            usb_bridge_app_deliver_data(bridge, UsbBridgePortDevice, (const uint8_t*) frame, len);
            mg_queue_del(&eth->eth_ifp.recv_queue, len);
        }
    }

    tud_task();
    bridge->st.open = tud_ready();
}

static void eth_app_send(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len)
{
    if (to & UsbBridgePortUsb)
    {
        usb_send(bridge, data, len);
    }
    if (to & UsbBridgePortDevice)
    {
        eth_send(bridge, data, len);
    }
}

const UsbBridgeBackend usb_bridge_eth_backend = {
    .unsupported = eth_unsupported,
    .start     = eth_backend_start,
    .stop      = eth_backend_stop,
    .on_events = eth_on_events,
    .app_send  = eth_app_send,
    .poll      = eth_poll,
};

#else

static const char* eth_unsupported(UsbBridgeMode mode)
{
    UNUSED(mode);
    return "not supported on this board: no Ethernet (RHS_HAL_ETH)";
}

const UsbBridgeBackend usb_bridge_eth_backend = {
    .unsupported = eth_unsupported,
};

#endif  // defined(RHS_HAL_ETH)
