#pragma once

#include "rhs.h"
#include "usb_bridge.h"

#define USB_BRIDGE_TX_BUF_SIZE 256

typedef enum
{
    UsbBridgeEvtStop          = (1 << 0),
    UsbBridgeEvtUsbRx         = (1 << 1),
    UsbBridgeEvtUsbTxComplete = (1 << 2),
    UsbBridgeEvtDeviceRx      = (1 << 3),
    UsbBridgeEvtDeviceError   = (1 << 4),
    UsbBridgeEvtAppTx         = (1 << 5),
    UsbBridgeEvtLineCoding    = (1 << 6),
} UsbBridgeEvt;

/** Data queued by *_send() from another thread: a CAN frame or bytes */
typedef struct
{
    uint8_t to;
    size_t  len;
    uint8_t data[];
} UsbBridgeAppMsg;

/** Target specific part, all calls except unsupported() run in the bridge thread */
typedef struct
{
    /** NULL when the target can run in @p mode on this board, otherwise the reason */
    const char* (*unsupported)(UsbBridgeMode mode);
    /** Bring USB and the device up, allocate bridge->backend */
    void (*start)(UsbBridge* bridge);
    /** Release USB and the device, free bridge->backend */
    void (*stop)(UsbBridge* bridge);
    /** Worker events for the backend: UsbBridgeEvtUsbRx, UsbBridgeEvtDeviceRx, UsbBridgeEvtDeviceError,
     * UsbBridgeEvtLineCoding */
    void (*on_events)(UsbBridge* bridge, uint32_t events);
    /** Send application data (a frame or bytes) to the @p to ports */
    void (*app_send)(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len);
    /** Called on every worker iteration when not NULL: the worker then polls instead of sleeping (Eth runs TinyUSB) */
    void (*poll)(UsbBridge* bridge);
} UsbBridgeBackend;

struct UsbBridge
{
    UsbBridgeConfig         cfg;
    const UsbBridgeBackend* backend;
    void*                   backend_ctx;

    RHSThread*       thread;
    RHSMessageQueue* app_queue; /**< UsbBridgeAppMsg* */

    UsbBridgeCanCallback  can_callback;
    UsbBridgeDataCallback data_callback;
    void*                 app_context;

    UsbBridgeState st;

    /* USB CDC transport */
    char   tx_buf[USB_BRIDGE_TX_BUF_SIZE];
    size_t tx_len;
};

extern const UsbBridgeBackend usb_bridge_can_backend;
extern const UsbBridgeBackend usb_bridge_serial_backend;
extern const UsbBridgeBackend usb_bridge_eth_backend;

/** Wake the bridge thread with @p events, also from an ISR */
void usb_bridge_notify(UsbBridge* bridge, uint32_t events);

/** True when the mode uses the device (Bridge, BridgeApp) */
static inline bool usb_bridge_has_device(UsbBridge* bridge)
{
    return bridge->cfg.mode != UsbBridgeModeApp;
}

/** True when the mode passes data to the application (App, BridgeApp) */
static inline bool usb_bridge_has_app(UsbBridge* bridge)
{
    return bridge->cfg.mode != UsbBridgeModeBridge;
}

/* USB CDC transport (CAN, RS232, RS485) */

/** Take the CDC port and set the worker's CDC callbacks */
void usb_bridge_cdc_start(UsbBridge* bridge);

void usb_bridge_cdc_stop(UsbBridge* bridge);

/** Buffered write to the host, sent by usb_bridge_cdc_flush() or when the buffer is full */
void usb_bridge_cdc_write(UsbBridge* bridge, const void* data, size_t len);

/** Send the buffer, waiting for the host up to a timeout: what does not fit is dropped (usb_tx_drops) */
void usb_bridge_cdc_flush(UsbBridge* bridge);

/* Application */

void usb_bridge_app_deliver_can(UsbBridge* bridge, UsbBridgePort from, const RHSHalCANFrameType* frame);

void usb_bridge_app_deliver_data(UsbBridge* bridge, UsbBridgePort from, const uint8_t* data, size_t len);
