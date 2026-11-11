/* usb_bridge CAN target: SLCAN (Lawicel) over USB CDC, classic CAN through rhs_hal_can.
 * The physical CAN starts on SLCAN O and stops on C, frames go to the host only while the channel is open. */
#include "usb_bridge_i.h"
#include "slcan.h"
#include "rhs_hal_usb_cdc.h"

#define TAG "usb_bridge_can"

#define CAN_RX_QUEUE_LEN 32
#define CAN_TX_RETRIES 10
#define CAN_CHANNEL RHSHalCANId1

/* SLCAN V / v answers: hardware 1.0, software 1.0 */
#define SLCAN_HW_VERSION "V1010\r"
#define SLCAN_SW_VERSION "v1010\r"

/* SLCAN F status flags */
#define SLCAN_STATUS_RX_FULL (1 << 0)
#define SLCAN_STATUS_DATA_OVERRUN (1 << 3)
#define SLCAN_STATUS_BUS_ERROR (1 << 7)

typedef struct
{
    RHSMessageQueue* rx_queue; /**< RHSHalCANFrameType from the CAN RX ISR */
    bool             running;
    uint8_t          status_flags;
    SlcanReader      reader;
} UsbBridgeCan;

static UsbBridgeCan* can_ctx(UsbBridge* bridge)
{
    return (UsbBridgeCan*) bridge->backend_ctx;
}

/*********************************** USB ************************************/

static void usb_send_frame(UsbBridge* bridge, const RHSHalCANFrameType* frame)
{
    if (!bridge->st.open)
    {
        bridge->st.usb_tx_drops++;
        return;
    }
    char   line[SLCAN_FRAME_MAX];
    size_t len = slcan_format_frame(frame, line);
    if (len == 0)
    {
        bridge->st.usb_tx_drops++;
        return;
    }
    usb_bridge_cdc_write(bridge, line, len);
    bridge->st.usb_tx++;
}

/*********************************** CAN ************************************/

static void can_rx_isr(RHSHalCANId id, void* context)
{
    UsbBridge*         bridge = (UsbBridge*) context;
    RHSHalCANFrameType frame;

    if (!rhs_hal_can_rx(id, &frame))
    {
        return;
    }
    if (rhs_message_queue_put(can_ctx(bridge)->rx_queue, &frame, 0) != RHSStatusOk)
    {
        bridge->st.device_rx_drops++;
    }
    usb_bridge_notify(bridge, UsbBridgeEvtDeviceRx);
}

static void can_sce_isr(RHSHalCANId id, RHSHalCANSCEEvent event, void* context)
{
    UNUSED(id);
    UNUSED(event);
    UsbBridge* bridge = (UsbBridge*) context;
    bridge->st.device_errors++;
    usb_bridge_notify(bridge, UsbBridgeEvtDeviceError);
}

static void can_start(UsbBridge* bridge)
{
    UsbBridgeCan* can = can_ctx(bridge);
    if (!usb_bridge_has_device(bridge) || can->running)
    {
        return;
    }
    rhs_hal_can_init(CAN_CHANNEL, bridge->st.baudrate);
    rhs_hal_can_async_rx_start(CAN_CHANNEL, can_rx_isr, bridge);
    rhs_hal_can_async_sce(CAN_CHANNEL, can_sce_isr, bridge);
    can->running = true;
}

static void can_stop(UsbBridge* bridge)
{
    UsbBridgeCan* can = can_ctx(bridge);
    if (!can->running)
    {
        return;
    }
    rhs_hal_can_deinit(CAN_CHANNEL);
    rhs_message_queue_reset(can->rx_queue);
    can->running = false;
}

static bool can_send_frame(UsbBridge* bridge, const RHSHalCANFrameType* frame)
{
    if (!can_ctx(bridge)->running)
    {
        bridge->st.device_tx_errors++;
        return false;
    }
    RHSHalCANFrameType tx = *frame;
    for (uint32_t i = 0; i < CAN_TX_RETRIES; i++)
    {
        if (rhs_hal_can_tx(CAN_CHANNEL, &tx))
        {
            bridge->st.device_tx++;
            return true;
        }
        rhs_delay_ms(1);  // all TX mailboxes busy
    }
    bridge->st.device_tx_errors++;
    return false;
}

/*********************************** SLCAN ************************************/

static void slcan_status(UsbBridge* bridge)
{
    UsbBridgeCan* can    = can_ctx(bridge);
    char          line[] = "F00\r";
    uint8_t       flags  = can->status_flags;

    if (rhs_message_queue_get_space(can->rx_queue) == 0)
    {
        flags |= SLCAN_STATUS_RX_FULL;
    }
    line[1]           = "0123456789ABCDEF"[flags >> 4];
    line[2]           = "0123456789ABCDEF"[flags & 0xF];
    can->status_flags = 0;  // Lawicel: F clears the error flags
    usb_bridge_cdc_write(bridge, line, sizeof(line) - 1);
}

static void slcan_serial(UsbBridge* bridge)
{
    // N answers 4 characters: the last two UID bytes
    char           line[] = "N0000\r";
    const uint8_t* uid    = rhs_hal_version_uid();
    for (size_t i = 0; i < 2; i++)
    {
        line[1 + i * 2] = "0123456789ABCDEF"[uid[10 + i] >> 4];
        line[2 + i * 2] = "0123456789ABCDEF"[uid[10 + i] & 0xF];
    }
    usb_bridge_cdc_write(bridge, line, sizeof(line) - 1);
}

static void slcan_handle(UsbBridge* bridge, const SlcanCmd* cmd)
{
    UsbBridgeCan* can = can_ctx(bridge);
    bool          ok  = true;

    switch (cmd->type)
    {
    case SlcanCmdNone:
        break;
    case SlcanCmdSetBitrate:
        if (bridge->st.open)
        {
            ok = false;
        }
        else
        {
            bridge->st.baudrate = cmd->bitrate;
        }
        break;
    case SlcanCmdOpen:
        if (bridge->st.open)
        {
            ok = false;
        }
        else
        {
            can_start(bridge);
            can->status_flags = 0;
            bridge->st.open   = true;
        }
        break;
    case SlcanCmdClose:
        // Close is accepted when already closed: hosts send C first to reset the adapter
        can_stop(bridge);
        bridge->st.open = false;
        break;
    case SlcanCmdTransmit:
        if (!bridge->st.open)
        {
            ok = false;
            break;
        }
        bridge->st.usb_rx++;
        if (usb_bridge_has_device(bridge))
        {
            ok = can_send_frame(bridge, &cmd->frame);
        }
        // Answer first: frames the app sends from its callback must follow the answer to this command
        if (ok)
        {
            usb_bridge_cdc_write(bridge, cmd->frame.type == FrameTypeExtID ? "Z\r" : "z\r", 2);
        }
        else
        {
            usb_bridge_cdc_write(bridge, SLCAN_ERROR, 1);
        }
        usb_bridge_app_deliver_can(bridge, UsbBridgePortUsb, &cmd->frame);
        return;
    case SlcanCmdStatus:
        if (!bridge->st.open)
        {
            ok = false;
            break;
        }
        slcan_status(bridge);
        return;
    case SlcanCmdVersion:
        usb_bridge_cdc_write(bridge, SLCAN_HW_VERSION, sizeof(SLCAN_HW_VERSION) - 1);
        return;
    case SlcanCmdSwVersion:
        usb_bridge_cdc_write(bridge, SLCAN_SW_VERSION, sizeof(SLCAN_SW_VERSION) - 1);
        return;
    case SlcanCmdSerial:
        slcan_serial(bridge);
        return;
    case SlcanCmdTimestamp:
        ok = !cmd->timestamp;  // timestamps are not supported, Z0 is
        break;
    case SlcanCmdAcceptFilter:
        break;
    case SlcanCmdListenOnly:  // rhs_hal_can has no silent mode
    case SlcanCmdInvalid:
    default:
        ok = false;
        break;
    }
    usb_bridge_cdc_write(bridge, ok ? SLCAN_OK : SLCAN_ERROR, 1);
}

/*********************************** BACKEND ************************************/

static const char* can_unsupported(UsbBridgeMode mode)
{
#if defined(STM32F103xE)
    // RM0008: USB and bxCAN share the dedicated 512-byte SRAM and cannot be used at the same time
    if (mode != UsbBridgeModeApp)
    {
        return "not supported on this board: CAN and USB share SRAM on STM32F103 and cannot work together, only app mode";
    }
#else
    UNUSED(mode);
#endif
    return NULL;
}

static void can_backend_start(UsbBridge* bridge)
{
    UsbBridgeCan* can = calloc(1, sizeof(UsbBridgeCan));
    can->rx_queue     = rhs_message_queue_alloc(CAN_RX_QUEUE_LEN, sizeof(RHSHalCANFrameType));
    slcan_reader_reset(&can->reader);
    bridge->backend_ctx = can;

    usb_bridge_cdc_start(bridge);
}

static void can_backend_stop(UsbBridge* bridge)
{
    UsbBridgeCan* can = can_ctx(bridge);
    can_stop(bridge);
    usb_bridge_cdc_stop(bridge);
    rhs_message_queue_free(can->rx_queue);
    free(can);
    bridge->backend_ctx = NULL;
}

static void can_on_events(UsbBridge* bridge, uint32_t events)
{
    UsbBridgeCan* can = can_ctx(bridge);

    if (events & UsbBridgeEvtDeviceError)
    {
        can->status_flags |= SLCAN_STATUS_BUS_ERROR;
    }
    if (events & UsbBridgeEvtDeviceRx)
    {
        if (bridge->st.device_rx_drops != 0)
        {
            can->status_flags |= SLCAN_STATUS_DATA_OVERRUN;
        }
        RHSHalCANFrameType frame;
        while (rhs_message_queue_get(can->rx_queue, &frame, 0) == RHSStatusOk)
        {
            bridge->st.device_rx++;
            usb_send_frame(bridge, &frame);
            usb_bridge_app_deliver_can(bridge, UsbBridgePortDevice, &frame);
        }
    }
    if (events & UsbBridgeEvtUsbRx)
    {
        uint8_t  data[CFG_TUD_CDC_RX_BUFSIZE];
        int32_t  len;
        SlcanCmd cmd;
        while ((len = rhs_hal_cdc_receive(data, sizeof(data))) > 0)
        {
            for (int32_t i = 0; i < len; i++)
            {
                if (slcan_reader_push(&can->reader, (char) data[i], &cmd))
                {
                    slcan_handle(bridge, &cmd);
                }
            }
        }
    }
}

static void can_app_send(UsbBridge* bridge, uint8_t to, const uint8_t* data, size_t len)
{
    rhs_assert(len == sizeof(RHSHalCANFrameType));
    const RHSHalCANFrameType* frame = (const RHSHalCANFrameType*) data;
    if (to & UsbBridgePortUsb)
    {
        usb_send_frame(bridge, frame);
    }
    if (to & UsbBridgePortDevice)
    {
        can_send_frame(bridge, frame);
    }
}

const UsbBridgeBackend usb_bridge_can_backend = {
    .unsupported = can_unsupported,
    .start     = can_backend_start,
    .stop      = can_backend_stop,
    .on_events = can_on_events,
    .app_send  = can_app_send,
};
