/* usb_bridge RS232 / RS485 targets: raw bytes over USB CDC, the UART through rhs_hal_serial.
 * The host sets the UART baud with the CDC line coding. Without RHS_HAL_SERIAL only App mode is available. */
#include "usb_bridge_i.h"
#include "rhs_hal_usb_cdc.h"

#define TAG "usb_bridge_serial"

#define SERIAL_CHUNK CFG_TUD_CDC_RX_BUFSIZE
#define SERIAL_RX_STREAM_SIZE (SERIAL_CHUNK * 8)

typedef struct
{
#if defined(RHS_HAL_SERIAL)
    RHSHalSerial*    serial;
    RHSStreamBuffer* rx_stream; /**< Bytes from the UART RX ISR */
#endif
    uint8_t buf[SERIAL_CHUNK];
} UsbBridgeSerial;

static UsbBridgeSerial* serial_ctx(UsbBridge* bridge)
{
    return (UsbBridgeSerial*) bridge->backend_ctx;
}

/*********************************** UART ************************************/

#if defined(RHS_HAL_SERIAL)

static void uart_rx_isr(RHSHalSerial* handle, RHSHalSerialRxEvent event, void* context)
{
    UsbBridge* bridge = (UsbBridge*) context;
    if (event & RHSHalSerialRxEventData)
    {
        uint8_t data = rhs_hal_serial_async_rx(handle);
        if (rhs_stream_buffer_send(serial_ctx(bridge)->rx_stream, &data, 1, 0) != 1)
        {
            bridge->st.device_rx_drops++;
        }
        usb_bridge_notify(bridge, UsbBridgeEvtDeviceRx);
    }
}

static void uart_start(UsbBridge* bridge)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);
    RHSHalSerialId   id     = bridge->cfg.target == UsbBridgeTargetRs232 ? RHSHalSerialIdRS232 : RHSHalSerialIdRS485;
    serial->serial          = rhs_hal_serial_init(id, bridge->st.baudrate);
    rhs_hal_serial_async_rx_start(serial->serial, uart_rx_isr, bridge);
}

static void uart_stop(UsbBridge* bridge)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);
    if (serial->serial != NULL)
    {
        rhs_hal_serial_deinit(serial->serial);
        serial->serial = NULL;
    }
}

static void uart_write(UsbBridge* bridge, const uint8_t* data, size_t len)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);
    while (len > 0)
    {
        uint16_t chunk = len > UINT16_MAX ? UINT16_MAX : (uint16_t) len;
        rhs_hal_serial_tx(serial->serial, data, chunk);
        bridge->st.device_tx += chunk;
        data += chunk;
        len -= chunk;
    }
}

static void uart_read(UsbBridge* bridge)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);
    size_t           len;
    while ((len = rhs_stream_buffer_receive(serial->rx_stream, serial->buf, sizeof(serial->buf), 0)) > 0)
    {
        bridge->st.device_rx += len;
        usb_bridge_cdc_write(bridge, serial->buf, len);
        bridge->st.usb_tx += len;
        usb_bridge_app_deliver_data(bridge, UsbBridgePortDevice, serial->buf, len);
    }
}

#endif

/*********************************** BACKEND ************************************/

static const char* serial_unsupported(UsbBridgeMode mode)
{
#if defined(RHS_HAL_SERIAL)
    UNUSED(mode);
    return NULL;
#else
    return mode == UsbBridgeModeApp ? NULL : "not supported on this board: no UART (RHS_HAL_SERIAL), only app mode";
#endif
}

static void serial_backend_start(UsbBridge* bridge)
{
    UsbBridgeSerial* serial = calloc(1, sizeof(UsbBridgeSerial));
    bridge->backend_ctx     = serial;

#if defined(RHS_HAL_SERIAL)
    serial->rx_stream = rhs_stream_buffer_alloc(SERIAL_RX_STREAM_SIZE, 1);
    if (usb_bridge_has_device(bridge))
    {
        uart_start(bridge);
    }
#endif
    usb_bridge_cdc_start(bridge);
}

static void serial_backend_stop(UsbBridge* bridge)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);
    usb_bridge_cdc_stop(bridge);
#if defined(RHS_HAL_SERIAL)
    uart_stop(bridge);
    rhs_stream_buffer_free(serial->rx_stream);
#endif
    free(serial);
    bridge->backend_ctx = NULL;
}

static void serial_on_events(UsbBridge* bridge, uint32_t events)
{
    UsbBridgeSerial* serial = serial_ctx(bridge);

    if (events & UsbBridgeEvtLineCoding)
    {
        uint32_t baudrate = rhs_hal_cdc_get_port_settings()->bit_rate;
        if (baudrate != 0 && baudrate != bridge->st.baudrate)
        {
            bridge->st.baudrate = baudrate;
#if defined(RHS_HAL_SERIAL)
            if (usb_bridge_has_device(bridge))
            {
                uart_stop(bridge);
                uart_start(bridge);
            }
#endif
            RHS_LOG_I(TAG, "Baudrate %lu", (unsigned long) baudrate);
        }
    }
#if defined(RHS_HAL_SERIAL)
    if (events & UsbBridgeEvtDeviceRx)
    {
        uart_read(bridge);
    }
#endif
    if (events & UsbBridgeEvtUsbRx)
    {
        int32_t len;
        while ((len = rhs_hal_cdc_receive(serial->buf, sizeof(serial->buf))) > 0)
        {
            bridge->st.usb_rx += (uint32_t) len;
#if defined(RHS_HAL_SERIAL)
            if (usb_bridge_has_device(bridge))
            {
                uart_write(bridge, serial->buf, (size_t) len);
            }
#endif
            usb_bridge_app_deliver_data(bridge, UsbBridgePortUsb, serial->buf, (size_t) len);
        }
    }
}

static void serial_app_send(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len)
{
    if (to & UsbBridgePortUsb)
    {
        usb_bridge_cdc_write(bridge, data, len);
        bridge->st.usb_tx += len;
    }
#if defined(RHS_HAL_SERIAL)
    if (to & UsbBridgePortDevice)
    {
        uart_write(bridge, data, len);
    }
#endif
}

const UsbBridgeBackend usb_bridge_serial_backend = {
    .unsupported = serial_unsupported,
    .start     = serial_backend_start,
    .stop      = serial_backend_stop,
    .on_events = serial_on_events,
    .app_send  = serial_app_send,
};
