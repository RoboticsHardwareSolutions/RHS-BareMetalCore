/* usb_bridge core: one bridge at a time, the bridge thread, the USB CDC transport, the application queue.
 * Target specific parts are UsbBridgeBackend in usb_bridge_can.c, usb_bridge_serial.c, usb_bridge_eth.c. */
#include "usb_bridge_i.h"
#include "rhs_hal_usb_cdc.h"

#define TAG "usb_bridge"

#define USB_BRIDGE_STACK_SIZE 2048
#define USB_BRIDGE_APP_QUEUE_LEN 16
#define USB_BRIDGE_USB_TX_TIMEOUT_MS 100

#define USB_BRIDGE_ALL_EVENTS                                                                                 \
    (UsbBridgeEvtStop | UsbBridgeEvtUsbRx | UsbBridgeEvtDeviceRx | UsbBridgeEvtDeviceError | UsbBridgeEvtAppTx | \
     UsbBridgeEvtLineCoding)
#define USB_BRIDGE_BACKEND_EVENTS \
    (UsbBridgeEvtUsbRx | UsbBridgeEvtDeviceRx | UsbBridgeEvtDeviceError | UsbBridgeEvtLineCoding)

static bool usb_bridge_running = false;

static const UsbBridgeBackend* usb_bridge_backend(UsbBridgeTarget target)
{
    switch (target)
    {
    case UsbBridgeTargetCan:
        return &usb_bridge_can_backend;
    case UsbBridgeTargetRs232:
    case UsbBridgeTargetRs485:
        return &usb_bridge_serial_backend;
    case UsbBridgeTargetEth:
        return &usb_bridge_eth_backend;
    default:
        return NULL;
    }
}

const char* usb_bridge_target_name(UsbBridgeTarget target)
{
    static const char* const names[] = {"can", "rs232", "rs485", "eth"};
    return target < sizeof(names) / sizeof(names[0]) ? names[target] : "?";
}

const char* usb_bridge_mode_name(UsbBridgeMode mode)
{
    static const char* const names[] = {"bridge", "app", "both"};
    return mode < sizeof(names) / sizeof(names[0]) ? names[mode] : "?";
}

/* Frames for CAN and Eth, bytes for serial */
static uint32_t usb_bridge_units(UsbBridge* bridge, size_t len)
{
    return (bridge->cfg.target == UsbBridgeTargetRs232 || bridge->cfg.target == UsbBridgeTargetRs485) ? len : 1;
}

void usb_bridge_notify(UsbBridge* bridge, uint32_t events)
{
    rhs_thread_flags_set(rhs_thread_get_id(bridge->thread), events);
}

/*********************************** USB CDC ************************************/

static void cdc_on_tx_complete(void* context)
{
    usb_bridge_notify((UsbBridge*) context, UsbBridgeEvtUsbTxComplete);
}

static void cdc_on_rx(void* context)
{
    usb_bridge_notify((UsbBridge*) context, UsbBridgeEvtUsbRx);
}

static void cdc_on_ctrl_line(void* context, CdcCtrlLine ctrl_lines)
{
    UsbBridge* bridge = (UsbBridge*) context;
    if (bridge->cfg.target != UsbBridgeTargetCan)
    {
        bridge->st.open = (ctrl_lines & CdcCtrlLineDTR) != 0;
    }
}

static void cdc_on_line_config(void* context, cdc_line_coding_t const* config)
{
    UNUSED(config);
    usb_bridge_notify((UsbBridge*) context, UsbBridgeEvtLineCoding);
}

static const CdcCallbacks cdc_callbacks = {
    .tx_callback          = cdc_on_tx_complete,
    .rx_callback          = cdc_on_rx,
    .ctrl_line_callback   = cdc_on_ctrl_line,
    .line_config_callback = cdc_on_line_config,
};

void usb_bridge_cdc_start(UsbBridge* bridge)
{
    rhs_assert(rhs_hal_cdc_acquire());
    rhs_hal_cdc_set_callbacks((CdcCallbacks*) &cdc_callbacks, bridge);
    // Data may already be waiting in the CDC FIFO
    usb_bridge_notify(bridge, UsbBridgeEvtUsbRx);
}

void usb_bridge_cdc_stop(UsbBridge* bridge)
{
    UNUSED(bridge);
    rhs_hal_cdc_release();
}

void usb_bridge_cdc_flush(UsbBridge* bridge)
{
    size_t   sent     = 0;
    uint32_t deadline = rhs_get_tick() + USB_BRIDGE_USB_TX_TIMEOUT_MS;

    while (sent < bridge->tx_len)
    {
        sent += rhs_hal_cdc_send((uint8_t*) &bridge->tx_buf[sent], bridge->tx_len - sent);
        if (sent < bridge->tx_len)
        {
            // TX FIFO full: wait until the host takes a packet
            if ((int32_t) (deadline - rhs_get_tick()) <= 0)
            {
                RHS_LOG_D(TAG, "USB TX timeout, %u bytes dropped", (unsigned) (bridge->tx_len - sent));
                bridge->st.usb_tx_drops += usb_bridge_units(bridge, bridge->tx_len - sent);
                break;
            }
            rhs_thread_flags_wait(UsbBridgeEvtUsbTxComplete, RHSFlagWaitAny, 10);
        }
    }
    bridge->tx_len = 0;
}

void usb_bridge_cdc_write(UsbBridge* bridge, const void* data, size_t len)
{
    const uint8_t* bytes = (const uint8_t*) data;
    while (len > 0)
    {
        if (bridge->tx_len == sizeof(bridge->tx_buf))
        {
            usb_bridge_cdc_flush(bridge);
        }
        size_t chunk = sizeof(bridge->tx_buf) - bridge->tx_len;
        if (chunk > len)
        {
            chunk = len;
        }
        memcpy(&bridge->tx_buf[bridge->tx_len], bytes, chunk);
        bridge->tx_len += chunk;
        bytes += chunk;
        len -= chunk;
    }
}

/*********************************** APPLICATION ************************************/

void usb_bridge_app_deliver_can(UsbBridge* bridge, UsbBridgePort from, const RHSHalCANFrameType* frame)
{
    if (usb_bridge_has_app(bridge) && bridge->can_callback != NULL)
    {
        bridge->st.app_rx++;
        bridge->can_callback(from, frame, bridge->app_context);
    }
}

void usb_bridge_app_deliver_data(UsbBridge* bridge, UsbBridgePort from, const uint8_t* data, size_t len)
{
    if (usb_bridge_has_app(bridge) && bridge->data_callback != NULL)
    {
        bridge->st.app_rx += usb_bridge_units(bridge, len);
        bridge->data_callback(from, data, len, bridge->app_context);
    }
}

static bool usb_bridge_app_send(UsbBridge* bridge, uint8_t to, const void* data, size_t len)
{
    to &= (UsbBridgePortUsb | UsbBridgePortDevice);
    if (to == 0 || ((to & UsbBridgePortDevice) && !usb_bridge_has_device(bridge)))
    {
        return false;
    }

    if (rhs_thread_get_current_id() == rhs_thread_get_id(bridge->thread))
    {
        // From an app callback: the worker is this thread, send right away
        bridge->st.app_tx += usb_bridge_units(bridge, len);
        bridge->backend->app_send(bridge, to, (const uint8_t*) data, len);
        return true;
    }

    UsbBridgeAppMsg* msg = malloc(sizeof(UsbBridgeAppMsg) + len);
    msg->to              = to;
    msg->len             = len;
    memcpy(msg->data, data, len);
    if (rhs_message_queue_put(bridge->app_queue, &msg, 0) != RHSStatusOk)
    {
        free(msg);
        return false;
    }
    usb_bridge_notify(bridge, UsbBridgeEvtAppTx);
    return true;
}

static void usb_bridge_app_queue_drain(UsbBridge* bridge, bool send)
{
    UsbBridgeAppMsg* msg;
    while (rhs_message_queue_get(bridge->app_queue, &msg, 0) == RHSStatusOk)
    {
        if (send)
        {
            bridge->st.app_tx += usb_bridge_units(bridge, msg->len);
            bridge->backend->app_send(bridge, msg->to, msg->data, msg->len);
        }
        free(msg);
    }
}

void usb_bridge_set_can_callback(UsbBridge* bridge, UsbBridgeCanCallback callback, void* context)
{
    rhs_assert(bridge);
    rhs_assert(bridge->cfg.target == UsbBridgeTargetCan);
    RHS_CRITICAL_ENTER();
    bridge->can_callback = callback;
    bridge->app_context  = context;
    RHS_CRITICAL_EXIT();
}

bool usb_bridge_can_send(UsbBridge* bridge, uint8_t to, const RHSHalCANFrameType* frame)
{
    rhs_assert(bridge);
    rhs_assert(frame);
    rhs_assert(bridge->cfg.target == UsbBridgeTargetCan);
    return usb_bridge_app_send(bridge, to, frame, sizeof(*frame));
}

void usb_bridge_set_data_callback(UsbBridge* bridge, UsbBridgeDataCallback callback, void* context)
{
    rhs_assert(bridge);
    rhs_assert(bridge->cfg.target != UsbBridgeTargetCan);
    RHS_CRITICAL_ENTER();
    bridge->data_callback = callback;
    bridge->app_context   = context;
    RHS_CRITICAL_EXIT();
}

bool usb_bridge_data_send(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len)
{
    rhs_assert(bridge);
    rhs_assert(data || len == 0);
    rhs_assert(bridge->cfg.target != UsbBridgeTargetCan);
    if (len == 0)
    {
        return true;
    }
    return usb_bridge_app_send(bridge, to, data, len);
}

/*********************************** WORKER ************************************/

static int32_t usb_bridge_worker(void* context)
{
    UsbBridge* bridge = (UsbBridge*) context;

    bridge->backend->start(bridge);

    while (1)
    {
        uint32_t events;
        if (bridge->backend->poll != NULL)
        {
            events = rhs_thread_flags_wait(USB_BRIDGE_ALL_EVENTS, RHSFlagWaitAny, 0);
            if (events & RHSFlagError)
            {
                events = 0;  // timeout: nothing happened
            }
        }
        else
        {
            events = rhs_thread_flags_wait(USB_BRIDGE_ALL_EVENTS, RHSFlagWaitAny, RHSWaitForever);
            rhs_assert(!(events & RHSFlagError));
        }

        if (events & UsbBridgeEvtStop)
        {
            break;
        }
        if (events & USB_BRIDGE_BACKEND_EVENTS)
        {
            bridge->backend->on_events(bridge, events);
        }
        if (events & UsbBridgeEvtAppTx)
        {
            usb_bridge_app_queue_drain(bridge, true);
        }
        if (bridge->backend->poll != NULL)
        {
            bridge->backend->poll(bridge);
            rhs_delay_tick(0);
        }
        usb_bridge_cdc_flush(bridge);
    }

    bridge->backend->stop(bridge);
    return 0;
}

/*********************************** API ************************************/

const char* usb_bridge_check(const UsbBridgeConfig* cfg)
{
    rhs_assert(cfg);
    const UsbBridgeBackend* backend = usb_bridge_backend(cfg->target);
    rhs_assert(backend);

    if (!rhs_hal_usb_is_available())
    {
        return "USB is not available (BMPLC_XL: strap PG10 low gives PA11/PA12 to CAN1)";
    }
    const char* reason = backend->unsupported(cfg->mode);
    if (reason != NULL)
    {
        return reason;
    }
    if (usb_bridge_running)
    {
        return "another bridge is running, one bridge at a time";
    }
    return NULL;
}

UsbBridge* usb_bridge_start(const UsbBridgeConfig* cfg)
{
    const char* reason = usb_bridge_check(cfg);
    if (reason != NULL)
    {
        RHS_LOG_E(TAG,
                  "%s %s not started: %s",
                  usb_bridge_target_name(cfg->target),
                  usb_bridge_mode_name(cfg->mode),
                  reason);
        return NULL;
    }

    bool busy;
    RHS_CRITICAL_ENTER();
    busy               = usb_bridge_running;
    usb_bridge_running = true;
    RHS_CRITICAL_EXIT();
    if (busy)
    {
        RHS_LOG_E(TAG, "Another bridge is running");
        return NULL;
    }

    UsbBridge* bridge   = calloc(1, sizeof(UsbBridge));
    bridge->cfg         = *cfg;
    bridge->backend     = usb_bridge_backend(cfg->target);
    bridge->st.baudrate = cfg->baudrate;
    bridge->app_queue   = rhs_message_queue_alloc(USB_BRIDGE_APP_QUEUE_LEN, sizeof(UsbBridgeAppMsg*));

    bridge->thread = rhs_thread_alloc("UsbBridge", USB_BRIDGE_STACK_SIZE, usb_bridge_worker, bridge);
    rhs_thread_start(bridge->thread);

    return bridge;
}

void usb_bridge_stop(UsbBridge* bridge)
{
    rhs_assert(bridge);
    usb_bridge_notify(bridge, UsbBridgeEvtStop);
    rhs_thread_join(bridge->thread);
    rhs_thread_free(bridge->thread);

    usb_bridge_app_queue_drain(bridge, false);
    rhs_message_queue_free(bridge->app_queue);
    free(bridge);

    usb_bridge_running = false;
}

void usb_bridge_get_config(UsbBridge* bridge, UsbBridgeConfig* cfg)
{
    rhs_assert(bridge);
    rhs_assert(cfg);
    *cfg = bridge->cfg;
}

void usb_bridge_get_state(UsbBridge* bridge, UsbBridgeState* state)
{
    rhs_assert(bridge);
    rhs_assert(state);
    memcpy(state, &bridge->st, sizeof(UsbBridgeState));
}
