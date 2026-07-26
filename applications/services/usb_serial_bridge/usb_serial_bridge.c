#include "rhs.h"
#include "rhs_hal.h"
#include "usb_serial_bridge.h"
#include "cli.h"
#include "tusb.h"
#include "rhs_hal_usb_cdc.h"

#define TAG "UsbSerialBridge"

#define USB_CDC_PKT_LEN CFG_TUD_CDC_RX_BUFSIZE
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
    RHSHalSerial*    serial_handle;

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
static void vcp_on_line_config(void* context, cdc_line_coding_t const* config);

static const CdcCallbacks cdc_cb = {
    vcp_on_cdc_tx_complete,
    vcp_on_cdc_rx,
    vcp_state_callback,
    vcp_on_cdc_control_line,
    vcp_on_line_config,
};

static void serial_rx_cb(RHSHalSerial* handle, RHSHalSerialRxEvent event, void* context)
{
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;

    if (event & (RHSHalSerialRxEventData))
    {
        uint8_t data = rhs_hal_serial_async_rx(handle);
        rhs_stream_buffer_send(usb_serial->rx_stream, &data, 1, 0);
        rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtRxDone);
    }
}

static void usb_serial_vcp_init(UsbSerialBridge* usb_serial, uint8_t vcp_ch)
{
    if (vcp_ch == 0)
    {
        rhs_hal_usb_set_interface(&usb_single_cdc_desc);
    }
    else
    {
        rhs_hal_usb_set_interface(&usb_dual_cdc_desc);
    }
    rhs_hal_cdc_set_callbacks(vcp_ch, (CdcCallbacks*) &cdc_cb, usb_serial);
}

static void usb_serial_vcp_deinit(UsbSerialBridge* usb_serial, uint8_t vcp_ch)
{
    rhs_hal_cdc_set_callbacks(vcp_ch, NULL, NULL);
    RHSHalUsbInterface* iface = rhs_hal_usb_get_interface();
    if (iface == &usb_dual_cdc_desc)
    {
        if (vcp_ch == 1)
        {
            rhs_hal_usb_set_interface(&usb_single_cdc_desc);
        }
    }
    if (iface == &usb_single_cdc_desc)
    {
        if (vcp_ch == 0)
        {
            rhs_hal_usb_set_interface(NULL);
        }
    }
    // If there is dual cdc interface and vcp_ch is 0, we can't switch to single cdc interface,
    // because it will break the second cdc interface.
    // So we just leave the dual cdc interface as is.
}

static void usb_uart_serial_init(UsbSerialBridge* usb_uart, uint32_t ch)
{
    usb_uart->serial_handle = rhs_hal_serial_init(ch, usb_uart->cfg.baudrate);
    rhs_hal_serial_async_rx_start(usb_uart->serial_handle, serial_rx_cb, usb_uart);
}

static void usb_uart_serial_deinit(UsbSerialBridge* usb_uart)
{
    rhs_hal_serial_deinit(usb_uart->serial_handle);
    usb_uart->serial_handle = NULL;
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

                rhs_hal_serial_tx(usb_serial->serial_handle, data, len);

                if (usb_serial->cfg.software_de_re != 0)
                {
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

    usb_uart_serial_init(usb_serial, usb_serial->cfg.serial_ch);
    usb_serial->st.baudrate_cur = usb_serial->cfg.baudrate;

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
            size_t len = rhs_stream_buffer_receive(usb_serial->rx_stream, usb_serial->rx_buf, USB_CDC_PKT_LEN, 0);
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
                    RHS_LOG_D(TAG, "USB TX timeout");
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

                usb_serial_vcp_deinit(usb_serial, usb_serial->cfg.vcp_ch);
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

                usb_uart_serial_deinit(usb_serial);
                usb_serial->cfg.serial_ch = usb_serial->cfg_new.serial_ch;
                usb_uart_serial_init(usb_serial, usb_serial->cfg.serial_ch);

                rhs_thread_start(usb_serial->tx_thread);
            }
            if (usb_serial->cfg.baudrate != usb_serial->cfg_new.baudrate)
            {
                rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtTxStop);
                rhs_thread_join(usb_serial->tx_thread);

                usb_uart_serial_deinit(usb_serial);

                usb_serial->cfg.baudrate    = usb_serial->cfg_new.baudrate;
                usb_serial->st.baudrate_cur = usb_serial->cfg_new.baudrate;
                usb_uart_serial_init(usb_serial, usb_serial->cfg.serial_ch);

                rhs_thread_start(usb_serial->tx_thread);
            }
            if (usb_serial->cfg.flow_pins != usb_serial->cfg_new.flow_pins)
            {
                rhs_crash("Flow pins change not implemented");
                usb_serial->cfg.flow_pins = usb_serial->cfg_new.flow_pins;
                events |= WorkerEvtCtrlLineSet;
            }
            if (usb_serial->cfg.software_de_re != usb_serial->cfg_new.software_de_re)
            {
                rhs_crash("Software de-re change not implemented");
                usb_serial->cfg.software_de_re = usb_serial->cfg_new.software_de_re;
            }
            api_lock_unlock(usb_serial->cfg_lock);
        }
        if (events & WorkerEvtLineCfgSet)
        {
            cdc_line_coding_t* coding       = rhs_hal_cdc_get_port_settings(usb_serial->cfg.vcp_ch);
            uint32_t           new_baudrate = coding->bit_rate;
            if (new_baudrate != 0 && new_baudrate != usb_serial->st.baudrate_cur)
            {
                usb_uart_serial_deinit(usb_serial);
                usb_serial->serial_handle = rhs_hal_serial_init(usb_serial->cfg.serial_ch, new_baudrate);
                rhs_hal_serial_async_rx_start(usb_serial->serial_handle, serial_rx_cb, usb_serial);

                usb_serial->cfg.baudrate    = new_baudrate;
                usb_serial->st.baudrate_cur = new_baudrate;
                RHS_LOG_I(TAG, "Baudrate changed to %lu", (unsigned long) new_baudrate);
            }
        }
        if (events & WorkerEvtCtrlLineSet)
        {
            /* Control line state changed — no action needed */
        }
    }

    usb_serial_vcp_deinit(usb_serial, usb_serial->cfg.vcp_ch);
    usb_uart_serial_deinit(usb_serial);

    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->tx_thread), WorkerEvtTxStop);
    rhs_thread_join(usb_serial->tx_thread);
    rhs_thread_free(usb_serial->tx_thread);

    rhs_stream_buffer_free(usb_serial->rx_stream);
    rhs_mutex_free(usb_serial->usb_mutex);
    rhs_semaphore_free(usb_serial->tx_sem);

    // rhs_hal_usb_unlock();
    // rhs_assert(rhs_hal_usb_set_config(&usb_cdc_single, NULL) == true);

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
    UNUSED(context);
    UNUSED(state);
}

static void vcp_on_cdc_control_line(void* context, uint8_t state)
{
    UNUSED(state);
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtCtrlLineSet);
}

static void vcp_on_line_config(void* context, cdc_line_coding_t const* config)
{
    UNUSED(config);
    UsbSerialBridge* usb_serial = (UsbSerialBridge*) context;
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtLineCfgSet);
}

static void usb_bridge_cb(char* args, void* context)
{
    static UsbSerialBridge* usb_rs232 = NULL; /* FIXME Sorry programming God */
    static UsbSerialBridge* usb_rs485 = NULL;
    if (args == NULL)
    {
        RHS_LOG_E(TAG, "Invalid argument");
        return;
    }

    bool want_rs232 = strstr(args, "-rs232") != NULL;
    bool want_rs485 = strstr(args, "-rs485") != NULL;

    if (!want_rs232 && !want_rs485)
    {
        RHS_LOG_E(TAG, "Invalid argument");
        return;
    }

    if (want_rs232)
    {
        if (usb_rs232 == NULL)
        {
            UsbSerialConfig cfg = {
                .vcp_ch         = usb_rs485 ? 1 : 0,
                .serial_ch      = RHSHalSerialIdRS232,
                .flow_pins      = 0,
                .baudrate_mode  = 0,
                .baudrate       = 9600,
                .software_de_re = 0,
            };
            usb_rs232 = usb_serial_enable(&cfg);
            RHS_LOG_W(TAG, "RS232 enabled");
        }
        else
        {
            usb_serial_disable(usb_rs232);
            usb_rs232 = NULL;
            RHS_LOG_W(TAG, "RS232 disabled");
        }
    }

    if (want_rs485)
    {
        if (usb_rs485 == NULL)
        {
            UsbSerialConfig cfg = {
                .vcp_ch         = usb_rs232 ? 1 : 0,
                .serial_ch      = RHSHalSerialIdRS485,
                .flow_pins      = 0,
                .baudrate_mode  = 0,
                .baudrate       = 9600,
                .software_de_re = 0,
            };
            usb_rs485 = usb_serial_enable(&cfg);
            RHS_LOG_W(TAG, "RS485 enabled");
        }
        else
        {
            usb_serial_disable(usb_rs485);
            usb_rs485 = NULL;
            RHS_LOG_W(TAG, "RS485 disabled");
        }
    }
}

UsbSerialBridge* usb_serial_enable(UsbSerialConfig* cfg)
{
    UsbSerialBridge* usb_serial = calloc(1, sizeof(UsbSerialBridge));
    memcpy(&(usb_serial->cfg_new), cfg, sizeof(UsbSerialConfig));

    usb_serial->thread = rhs_thread_alloc("UsbSerialWorker", 1024, usb_serial_worker, usb_serial);

    rhs_thread_start(usb_serial->thread);

    return usb_serial;
}

void usb_serial_disable(UsbSerialBridge* usb_serial)
{
    rhs_assert(usb_serial);
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtStop);
    rhs_thread_join(usb_serial->thread);
    rhs_thread_free(usb_serial->thread);
    free(usb_serial);
}

void usb_serial_set_config(UsbSerialBridge* usb_serial, UsbSerialConfig* cfg)
{
    rhs_assert(usb_serial);
    rhs_assert(cfg);
    usb_serial->cfg_lock = api_lock_alloc_locked();
    memcpy(&(usb_serial->cfg_new), cfg, sizeof(UsbSerialConfig));
    rhs_thread_flags_set(rhs_thread_get_id(usb_serial->thread), WorkerEvtCfgChange);
    api_lock_wait_unlock_and_free(usb_serial->cfg_lock);
}

void usb_serial_get_config(UsbSerialBridge* usb_serial, UsbSerialConfig* cfg)
{
    rhs_assert(usb_serial);
    rhs_assert(cfg);
    memcpy(cfg, &(usb_serial->cfg_new), sizeof(UsbSerialConfig));
}

void usb_serial_get_state(UsbSerialBridge* usb_serial, UsbSerialState* st)
{
    rhs_assert(usb_serial);
    rhs_assert(st);
    memcpy(st, &(usb_serial->st), sizeof(UsbSerialState));
}

void cli_vcp_start_up(void)
{
    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "usb_bridge", usb_bridge_cb, NULL);
    rhs_record_close(RECORD_CLI);

    // TODO cli_vcp
    // This is a stub
    // rhs_hal_usb_unlock();
    // rhs_assert(rhs_hal_usb_set_config(&usb_cdc_dual, NULL) == true);
    // rhs_hal_cdc_set_callbacks(0, (CdcCallbacks*) &cdc_cb, cli_vcp);
}
