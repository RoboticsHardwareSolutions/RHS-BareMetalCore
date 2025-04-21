#include "rhs.h"
#include "rhs_hal.h"
#include "usb_uart_bridge.h"
#include "cli.h"

#define TAG "UsbUartBridge"

#define USB_CDC_PKT_LEN CDC_DATA_SZ

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

struct UsbUartBridge
{
    UsbUartConfig cfg;
    UsbUartConfig cfg_new;

    RHSThread* thread;
    RHSThread* tx_thread;

    // RHSStreamBuffer* rx_stream;
    RHSHalSerialId serial_handle;

    RHSMutex* usb_mutex;

    // RHSSemaphore* tx_sem;

    UsbUartState st;

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

static void usb_uart_vcp_init(UsbUartBridge* usb_uart, uint8_t vcp_ch)
{
    rhs_hal_usb_unlock();
    rhs_assert(rhs_hal_usb_set_config(&usb_cdc_single, NULL) == true);
    rhs_hal_cdc_set_callbacks(vcp_ch, (CdcCallbacks*) &cdc_cb, usb_uart);
}

static void usb_uart_vcp_deinit(UsbUartBridge* usb_uart, uint8_t vcp_ch)
{
    rhs_hal_cdc_set_callbacks(vcp_ch, NULL, NULL);
    if (vcp_ch != 0)
    {
    }
}

void serial_rx_cb(RHSHalSerialRxEvent event, void* context)
{
    rhs_hal_serial_async_rx(RHSHalSerialIdRS232);
    // RHS_LOG_D(TAG, "rs232 rx %d", rhs_hal_serial_async_rx(RHSHalSerialIdRS232));
}

static int32_t usb_uart_tx_thread(void* context)
{
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;

    uint8_t data[USB_CDC_PKT_LEN];
    while (1)
    {
        uint32_t events = rhs_thread_flags_wait(WORKER_ALL_TX_EVENTS, RHSFlagWaitAny, RHSWaitForever);
        rhs_assert(!(events & RHSFlagError));
        if (events & WorkerEvtTxStop)
            break;
        if (events & WorkerEvtCdcRx)
        {
            rhs_assert(rhs_mutex_acquire(usb_uart->usb_mutex, RHSWaitForever) == RHSStatusOk);
            size_t len = rhs_hal_cdc_receive(usb_uart->cfg.vcp_ch, data, USB_CDC_PKT_LEN);
            rhs_assert(rhs_mutex_release(usb_uart->usb_mutex) == RHSStatusOk);

            if (len > 0)
            {
                usb_uart->st.tx_cnt += len;
                // RHS_LOG_D(TAG, "rx %d", len);

                // rhs_assert(rhs_mutex_acquire(usb_uart->usb_mutex, RHSWaitForever) == RHSStatusOk);
                // rhs_hal_cdc_send(usb_uart->cfg.vcp_ch, data, len);
                // rhs_assert(rhs_mutex_release(usb_uart->usb_mutex) == RHSStatusOk);

                // if(usb_uart->cfg.software_de_re != 0)
                // rhs_hal_gpio_write(USB_USART_DE_RE_PIN, false);
                rhs_hal_serial_tx(RHSHalSerialIdRS232, data, len);

                // rhs_hal_serial_tx(usb_uart->serial_handle, data, len);

                if (usb_uart->cfg.software_de_re != 0)
                {
                    // rhs_hal_serial_tx_wait_complete(usb_uart->serial_handle);
                    // rhs_hal_gpio_write(USB_USART_DE_RE_PIN, true);
                }
            }
        }
    }
    return 0;
}

static int32_t usb_uart_worker(void* context)
{
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;

    memcpy(&usb_uart->cfg, &usb_uart->cfg_new, sizeof(UsbUartConfig));

    // usb_uart->rx_stream = rhs_stream_buffer_alloc(USB_UART_RX_BUF_SIZE, 1);

    // usb_uart->tx_sem = rhs_semaphore_alloc(1, 1);
    usb_uart->usb_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);

    usb_uart->tx_thread = rhs_thread_alloc("UsbUartTxWorker", 1024, usb_uart_tx_thread, usb_uart);

    usb_uart_vcp_init(usb_uart, usb_uart->cfg.vcp_ch);

    rhs_hal_serial_init(RHSHalSerialIdRS232, 115200);
    rhs_hal_serial_async_rx_start(RHSHalSerialIdRS232, serial_rx_cb, usb_uart);
    // usb_uart_serial_init(usb_uart, usb_uart->cfg.uart_ch);
    // usb_uart_set_baudrate(usb_uart, usb_uart->cfg.baudrate);
    // if(usb_uart->cfg.flow_pins != 0) {
    //     rhs_assert((size_t)(usb_uart->cfg.flow_pins - 1) < COUNT_OF(flow_pins));
    //     rhs_hal_gpio_init_simple(
    //         flow_pins[usb_uart->cfg.flow_pins - 1][0], GpioModeOutputPushPull);
    //     rhs_hal_gpio_init_simple(
    //         flow_pins[usb_uart->cfg.flow_pins - 1][1], GpioModeOutputPushPull);
    //     usb_uart_update_ctrl_lines(usb_uart);
    // }

    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->tx_thread), WorkerEvtCdcRx);

    rhs_thread_start(usb_uart->tx_thread);

    while (1)
    {
        uint32_t events = rhs_thread_flags_wait(WORKER_ALL_RX_EVENTS, RHSFlagWaitAny, RHSWaitForever);
        rhs_assert(!(events & RHSFlagError));
        if (events & WorkerEvtStop)
            break;
        if (events & (WorkerEvtRxDone | WorkerEvtCdcTxComplete))
        {
            // size_t len = rhs_stream_buffer_receive(
            // usb_uart->rx_stream, usb_uart->rx_buf, USB_CDC_PKT_LEN, 0);
            // if(len > 0) {
            //     if(rhs_semaphore_acquire(usb_uart->tx_sem, 100) == RHSStatusOk) {
            //         usb_uart->st.rx_cnt += len;
            //         rhs_assert(
            //             rhs_mutex_acquire(usb_uart->usb_mutex, RHSWaitForever) == RHSStatusOk);
            //         rhs_hal_cdc_send(usb_uart->cfg.vcp_ch, usb_uart->rx_buf, len);
            //         rhs_assert(rhs_mutex_release(usb_uart->usb_mutex) == RHSStatusOk);
            //     } else {
            //         rhs_stream_buffer_reset(usb_uart->rx_stream);
            //     }
            // }
        }
        if (events & WorkerEvtCfgChange)
        {
            if (usb_uart->cfg.vcp_ch != usb_uart->cfg_new.vcp_ch)
            {
                rhs_thread_flags_set(rhs_thread_get_id(usb_uart->tx_thread), WorkerEvtTxStop);
                rhs_thread_join(usb_uart->tx_thread);

                // usb_uart_vcp_deinit(usb_uart, usb_uart->cfg.vcp_ch);
                usb_uart_vcp_init(usb_uart, usb_uart->cfg_new.vcp_ch);

                usb_uart->cfg.vcp_ch = usb_uart->cfg_new.vcp_ch;
                rhs_thread_start(usb_uart->tx_thread);
                events |= WorkerEvtCtrlLineSet;
                events |= WorkerEvtLineCfgSet;
            }
            if (usb_uart->cfg.uart_ch != usb_uart->cfg_new.uart_ch)
            {
                rhs_thread_flags_set(rhs_thread_get_id(usb_uart->tx_thread), WorkerEvtTxStop);
                rhs_thread_join(usb_uart->tx_thread);

                // usb_uart_serial_deinit(usb_uart);
                // usb_uart_serial_init(usb_uart, usb_uart->cfg_new.uart_ch);

                usb_uart->cfg.uart_ch = usb_uart->cfg_new.uart_ch;
                // usb_uart_set_baudrate(usb_uart, usb_uart->cfg.baudrate);

                rhs_thread_start(usb_uart->tx_thread);
            }
            if (usb_uart->cfg.baudrate != usb_uart->cfg_new.baudrate)
            {
                // usb_uart_set_baudrate(usb_uart, usb_uart->cfg_new.baudrate);
                usb_uart->cfg.baudrate = usb_uart->cfg_new.baudrate;
            }
            if (usb_uart->cfg.flow_pins != usb_uart->cfg_new.flow_pins)
            {
                // if(usb_uart->cfg.flow_pins != 0) {
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_uart->cfg.flow_pins - 1][0], GpioModeAnalog);
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_uart->cfg.flow_pins - 1][1], GpioModeAnalog);
                // }
                // if(usb_uart->cfg_new.flow_pins != 0) {
                //     rhs_assert((size_t)(usb_uart->cfg_new.flow_pins - 1) < COUNT_OF(flow_pins));
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_uart->cfg_new.flow_pins - 1][0], GpioModeOutputPushPull);
                //     rhs_hal_gpio_init_simple(
                //         flow_pins[usb_uart->cfg_new.flow_pins - 1][1], GpioModeOutputPushPull);
                // }
                usb_uart->cfg.flow_pins = usb_uart->cfg_new.flow_pins;
                events |= WorkerEvtCtrlLineSet;
            }
            if (usb_uart->cfg.software_de_re != usb_uart->cfg_new.software_de_re)
            {
                usb_uart->cfg.software_de_re = usb_uart->cfg_new.software_de_re;
                // if(usb_uart->cfg.software_de_re != 0) {
                //     rhs_hal_gpio_write(USB_USART_DE_RE_PIN, true);
                //     rhs_hal_gpio_init(
                //         USB_USART_DE_RE_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedMedium);
                // } else {
                //     rhs_hal_gpio_init(
                //         USB_USART_DE_RE_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
                // }
            }
            api_lock_unlock(usb_uart->cfg_lock);
        }
        if (events & WorkerEvtLineCfgSet)
        {
            if (usb_uart->cfg.baudrate == 0)
            {
                // usb_uart_set_baudrate(usb_uart, usb_uart->cfg.baudrate);}
            }
            if (events & WorkerEvtCtrlLineSet)
            {
                // usb_uart_update_ctrl_lines(usb_uart);
            }
        }
    }

    usb_uart_vcp_deinit(usb_uart, usb_uart->cfg.vcp_ch);
    // usb_uart_serial_deinit(usb_uart);

    // rhs_hal_gpio_init(USB_USART_DE_RE_PIN, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    // if(usb_uart->cfg.flow_pins != 0) {
    //     rhs_hal_gpio_init_simple(flow_pins[usb_uart->cfg.flow_pins - 1][0], GpioModeAnalog);
    //     rhs_hal_gpio_init_simple(flow_pins[usb_uart->cfg.flow_pins - 1][1], GpioModeAnalog);
    // }

    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->tx_thread), WorkerEvtTxStop);
    rhs_thread_join(usb_uart->tx_thread);
    rhs_thread_free(usb_uart->tx_thread);

    // rhs_stream_buffer_free(usb_uart->rx_stream);
    rhs_mutex_free(usb_uart->usb_mutex);
    // rhs_semaphore_free(usb_uart->tx_sem);

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
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;
    // rhs_semaphore_release(usb_uart->tx_sem);
    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->thread), WorkerEvtCdcTxComplete);
}

static void vcp_on_cdc_rx(void* context)
{
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->tx_thread), WorkerEvtCdcRx);
}

static void vcp_state_callback(void* context, uint8_t state)
{
    // UNUSED(context);
    // UNUSED(state);
}

static void vcp_on_cdc_control_line(void* context, uint8_t state)
{
    // UNUSED(state);
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->thread), WorkerEvtCtrlLineSet);
}

static void vcp_on_line_config(void* context, struct usb_cdc_line_coding* config)
{
    // UNUSED(config);
    UsbUartBridge* usb_uart = (UsbUartBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_uart->thread), WorkerEvtLineCfgSet);
}

UsbUartBridge* usb_uart_enable(UsbUartConfig* cfg)
{
    UsbUartBridge* usb_uart = malloc(sizeof(UsbUartBridge));

    memcpy(&(usb_uart->cfg_new), cfg, sizeof(UsbUartConfig));

    usb_uart->thread = rhs_thread_alloc("UsbUartWorker", 1024, usb_uart_worker, usb_uart);

    rhs_thread_start(usb_uart->thread);
    return usb_uart;
}

void usb_uart_bridge_start_up(void)
{
    UsbUartConfig* cfg = malloc(sizeof(UsbUartConfig));
    cfg->vcp_ch        = 0;
    cfg->uart_ch       = 0;
    cfg->flow_pins     = 0;
    cfg->baudrate_mode = 0;
    cfg->baudrate      = 0;
    usb_uart_enable(cfg);
}
