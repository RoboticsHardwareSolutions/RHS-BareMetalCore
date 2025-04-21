#include "rhs.h"
#include "rhs_hal.h"
#include "usb_serial_bridge.h"
#include "cli.h"

#define TAG "UsbSerialBridge"

#define USB_CDC_PKT_LEN CDC_DATA_SZ
#define USB_UART_RX_BUF_SIZE (USB_CDC_PKT_LEN * 5)

typedef enum
{
    WorkerEvtStop   = (1 << 0),
    WorkerEvtRxDone = (1 << 1),

    WorkerEvtTxStop        = (1 << 2),
    WorkerEvtCdcRx         = (1 << 3),
    WorkerEvtCdcTxComplete = (1 << 4),

    WorkerEvtCfgChange = (1 << 5),

    WorkerEvtLineCfgSet  = (1 << 6),
    WorkerEvtCtrlLineSet = (1 << 7),

} WorkerEvtFlags;

#define WORKER_ALL_RX_EVENTS                                                                             \
    (WorkerEvtStop | WorkerEvtRxDone | WorkerEvtCfgChange | WorkerEvtLineCfgSet | WorkerEvtCtrlLineSet | \
     WorkerEvtCdcTxComplete)
#define WORKER_ALL_TX_EVENTS (WorkerEvtTxStop | WorkerEvtCdcRx)

struct UsbSerialBridge
{
    UsbSerialConfig cfg;
    UsbSerialConfig cfg_new;

    RHSThread* thread;
    RHSThread* tx_thread;

    RHSStreamBuffer* rx_stream;
    RHSHalSerialId   serial_handle;

    RHSMutex* usb_mutex;

    RHSSemaphore* tx_sem;

    UsbSerialState st;

    RHSApiLock cfg_lock;

    uint8_t rx_buf[USB_CDC_PKT_LEN];
};

static void vcp_on_cdc_tx_complete(void* context);
static void vcp_on_cdc_rx(void* context);
static void vcp_state_callback(void* context, uint8_t state);
static void vcp_on_cdc_control_line(void* context, uint8_t state);
static void vcp_on_line_config(void* context, struct usb_cdc_line_coding* config);

static const CdcCallbacks cdc_cb = {
    vcp_on_cdc_tx_complete,
    vcp_on_cdc_rx,
    vcp_state_callback,
    vcp_on_cdc_control_line,
    vcp_on_line_config,
};

static void usb_serial_vcp_init(UsbSerialBridge* usb_serial, uint8_t vcp_ch)
{
    rhs_hal_usb_unlock();
    rhs_assert(rhs_hal_usb_set_config(&usb_cdc_single, NULL) == true);
    rhs_hal_cdc_set_callbacks(vcp_ch, (CdcCallbacks*) &cdc_cb, usb_serial);
}

static void usb_serial_vcp_deinit(UsbSerialBridge* usb_serial, uint8_t vcp_ch)
{
    rhs_hal_cdc_set_callbacks(vcp_ch, NULL, NULL);
    if (vcp_ch != 0)
    {
    }
}

void serial_rx_cb(RHSHalSerialRxEvent event, void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;

    rhs_hal_serial_async_rx(RHSHalSerialIdRS232);
    if (event & (RHSHalSerialRxEventData))
    {
        uint8_t data = rhs_hal_serial_async_rx(RHSHalSerialIdRS232);
        rhs_stream_buffer_send(usb_serial->rx_stream, &data, 1, 0);
        rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtRxDone);
    }
}

static int32_t usb_serial_tx_thread(void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;

    uint8_t data[USB_CDC_PKT_LEN];
    while (1)
    {
        uint32_t events = rhs_thread_flags_wait(WORKER_ALL_TX_EVENTS, RHSFlagWaitAny, RHSWaitForever);
        rhs_assert(!(events & RHSFlagError));
        if (events & WorkerEvtTxStop)
            break;
        if (events & WorkerEvtCdcRx)
        {
            rhs_assert(rhs_mutex_acquire(usb_serial->usb_mutex, RHSWaitForever) == RHSStatusOk);
            size_t len = rhs_hal_cdc_receive(usb_serial->cfg.vcp_ch, data, USB_CDC_PKT_LEN);
            rhs_assert(rhs_mutex_release(usb_serial->usb_mutex) == RHSStatusOk);

            if (len > 0)
            {
                usb_serial->st.tx_cnt += len;

                // if(usb_serial->cfg.software_de_re != 0)
                // rhs_hal_gpio_write(USB_USART_DE_RE_PIN, false);
                rhs_hal_serial_tx(RHSHalSerialIdRS232, data, len);

                if (usb_serial->cfg.software_de_re != 0)
                {
                    // rhs_hal_serial_tx_wait_complete(usb_serial->serial_handle);
                    // rhs_hal_gpio_write(USB_USART_DE_RE_PIN, true);
                }
            }
        }
    }
    return 0;
}

static int32_t usb_serial_worker(void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;

    memcpy(&usb_serial->cfg, &usb_serial->cfg_new, sizeof(UsbSerialConfig));

    usb_serial->rx_stream = rhs_stream_buffer_alloc(USB_UART_RX_BUF_SIZE, 1);

    usb_serial->tx_sem    = rhs_semaphore_alloc(1, 1);
    usb_serial->usb_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);

    usb_serial->tx_thread = rhs_thread_alloc("UsbSerialTxWorker", 1024, usb_serial_tx_thread, usb_serial);

    usb_serial_vcp_init(usb_serial, usb_serial->cfg.vcp_ch);

    rhs_hal_serial_init(RHSHalSerialIdRS232, 115200);
    rhs_hal_serial_async_rx_start(RHSHalSerialIdRS232, serial_rx_cb, usb_serial);
    // if(usb_serial->cfg.flow_pins != 0) {
    //     rhs_assert((size_t)(usb_serial->cfg.flow_pins - 1) < COUNT_OF(flow_pins));
    //     rhs_hal_gpio_init_simple(
    //         flow_pins[usb_serial->cfg.flow_pins - 1][0], GpioModeOutputPushPull);
    //     rhs_hal_gpio_init_simple(
    //         flow_pins[usb_serial->cfg.flow_pins - 1][1], GpioModeOutputPushPull);
    //     usb_serial_update_ctrl_lines(usb_serial);
    // }

    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtCdcRx);

    rhs_thread_start(usb_serial->tx_thread);

    while (1)
    {
        uint32_t events = rhs_thread_flags_wait(WORKER_ALL_RX_EVENTS, RHSFlagWaitAny, RHSWaitForever);
        rhs_assert(!(events & RHSFlagError));
        if (events & WorkerEvtStop)
            break;
        if (events & (WorkerEvtRxDone | WorkerEvtCdcTxComplete))
        {
            uint16_t len = rhs_stream_buffer_receive(usb_serial->rx_stream, usb_serial->rx_buf, USB_CDC_PKT_LEN, 0);
            if (len > 0)
            {
                if (rhs_semaphore_acquire(usb_serial->tx_sem, 100) == RHSStatusOk)
                {
                    usb_serial->st.rx_cnt += len;
                    rhs_assert(rhs_mutex_acquire(usb_serial->usb_mutex, RHSWaitForever) == RHSStatusOk);
                    rhs_hal_cdc_send(usb_serial->cfg.vcp_ch, usb_serial->rx_buf, len);
                    rhs_assert(rhs_mutex_release(usb_serial->usb_mutex) == RHSStatusOk);
                }
                else
                {
                    rhs_stream_buffer_reset(usb_serial->rx_stream);
                }
            }
        }
        if (events & WorkerEvtCfgChange)
        {
            if (usb_serial->cfg.vcp_ch != usb_serial->cfg_new.vcp_ch)
            {
                rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtTxStop);
                rhs_thread_join(usb_serial->tx_thread);

                // usb_serial_vcp_deinit(usb_serial, usb_serial->cfg.vcp_ch);
                usb_serial_vcp_init(usb_serial, usb_serial->cfg_new.vcp_ch);

                usb_serial->cfg.vcp_ch = usb_serial->cfg_new.vcp_ch;
                rhs_thread_start(usb_serial->tx_thread);
                events |= WorkerEvtCtrlLineSet;
                events |= WorkerEvtLineCfgSet;
            }
            if (usb_serial->cfg.serial_ch != usb_serial->cfg_new.serial_ch)
            {
                rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtTxStop);
                rhs_thread_join(usb_serial->tx_thread);

                // usb_serial_serial_deinit(usb_serial);
                // usb_serial_serial_init(usb_serial, usb_serial->cfg_new.serial_ch);

                usb_serial->cfg.serial_ch = usb_serial->cfg_new.serial_ch;
                // usb_serial_set_baudrate(usb_serial, usb_serial->cfg.baudrate);

                rhs_thread_start(usb_serial->tx_thread);
            }
            if (usb_serial->cfg.baudrate != usb_serial->cfg_new.baudrate)
            {
                // usb_serial_set_baudrate(usb_serial, usb_serial->cfg_new.baudrate);
                usb_serial->cfg.baudrate = usb_serial->cfg_new.baudrate;
            }
            if (usb_serial->cfg.flow_pins != usb_serial->cfg_new.flow_pins)
            {
                // if(usb_serial->cfg.flow_pins != 0) {
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_serial->cfg.flow_pins - 1][0], GpioModeAnalog);
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_serial->cfg.flow_pins - 1][1], GpioModeAnalog);
                // }
                // if(usb_serial->cfg_new.flow_pins != 0) {
                //     rhs_assert((size_t)(usb_serial->cfg_new.flow_pins - 1) < COUNT_OF(flow_pins));
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_serial->cfg_new.flow_pins - 1][0], GpioModeOutputPushPull);
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_serial->cfg_new.flow_pins - 1][1], GpioModeOutputPushPull);
                // }
                usb_serial->cfg.flow_pins = usb_serial->cfg_new.flow_pins;
                events |= WorkerEvtCtrlLineSet;
            }
            if (usb_serial->cfg.software_de_re != usb_serial->cfg_new.software_de_re)
            {
                usb_serial->cfg.software_de_re = usb_serial->cfg_new.software_de_re;
                // if(usb_serial->cfg.software_de_re != 0) {
                //     rhs_hal_gpio_write(USB_USART_DE_RE_PIN, true);
                //     rhs_hal_gpio_init(
                //         USB_USART_DE_RE_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedMedium);
                // } else {
                //     rhs_hal_gpio_init(
                //         USB_USART_DE_RE_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
                // }
            }
            api_lock_unlock(usb_serial->cfg_lock);
        }
        if (events & WorkerEvtLineCfgSet)
        {
            if (usb_serial->cfg.baudrate == 0)
            {
                // usb_serial_set_baudrate(usb_serial, usb_serial->cfg.baudrate);}
            }
            if (events & WorkerEvtCtrlLineSet)
            {
                // usb_serial_update_ctrl_lines(usb_serial);
            }
        }
    }

    usb_serial_vcp_deinit(usb_serial, usb_serial->cfg.vcp_ch);
    // usb_serial_serial_deinit(usb_serial);

    // rhs_hal_gpio_init(USB_USART_DE_RE_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    // if(usb_serial->cfg.flow_pins != 0) {
    //     rhs_hal_gpio_init_simple(flow_pins[usb_serial->cfg.flow_pins - 1][0], GpioModeAnalog);
    //     rhs_hal_gpio_init_simple(flow_pins[usb_serial->cfg.flow_pins - 1][1], GpioModeAnalog);
    // }

    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtTxStop);
    rhs_thread_join(usb_serial->tx_thread);
    rhs_thread_free(usb_serial->tx_thread);

    rhs_stream_buffer_free(usb_serial->rx_stream);
    rhs_mutex_free(usb_serial->usb_mutex);
    rhs_semaphore_free(usb_serial->tx_sem);

    rhs_hal_usb_unlock();
    rhs_assert(rhs_hal_usb_set_config(&usb_cdc_single, NULL) == true);
    // Cli* cli = rhs_record_open(RECORD_CLI);
    // cli_session_open(cli, &cli_vcp);
    // rhs_record_close(RECORD_CLI);

    return 0;
}

/* VCP callbacks */

static void vcp_on_cdc_tx_complete(void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_semaphore_release(usb_serial->tx_sem);
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtCdcTxComplete);
}

static void vcp_on_cdc_rx(void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtCdcRx);
}

static void vcp_state_callback(void* context, uint8_t state)
{
    // UNUSED(context);
    // UNUSED(state);
}

static void vcp_on_cdc_control_line(void* context, uint8_t state)
{
    // UNUSED(state);
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtCtrlLineSet);
}

static void vcp_on_line_config(void* context, struct usb_cdc_line_coding* config)
{
    // UNUSED(config);
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtLineCfgSet);
}

UsbSerialBridge* usb_serial_enable(UsbSerialConfig* cfg)
{
    UsbSerialBridge* usb_serial = malloc(sizeof(UsbSerialBridge));

    memcpy(&(usb_serial->cfg_new), cfg, sizeof(UsbSerialConfig));

    usb_serial->thread = rhs_thread_alloc("UsbSerialWorker", 1024, usb_serial_worker, usb_serial);

    rhs_thread_start(usb_serial->thread);
    return usb_serial;
}

void usb_serial_bridge_start_up(void)
{
    UsbSerialConfig* cfg = malloc(sizeof(UsbSerialConfig));
    cfg->vcp_ch          = 0;
    cfg->serial_ch       = 0;
    cfg->flow_pins       = 0;
    cfg->baudrate_mode   = 0;
    cfg->baudrate        = 0;
    usb_serial_enable(cfg);
}
