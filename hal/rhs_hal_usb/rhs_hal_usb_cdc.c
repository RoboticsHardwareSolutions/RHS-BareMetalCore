#include "rhs.h"
#include "rhs_hal.h"
#include "cli.h"
#include <ctype.h>

#define TAG "UsbCDC"

/* One CDC port, TinyUSB interface 0 */
#define CDC_ITF 0

enum
{
    ITF_CDC_NUM_CDC = 0,
    ITF_CDC_NUM_CDC_DATA,
    ITF_CDC_NUM_TOTAL
};

static tusb_desc_device_t const desc_device_cdc = {
    .bLength         = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB          = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    .bDeviceClass    = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor  = 0x0483,
    .idProduct = 0x5740,
    .bcdDevice = 0x0101,

    .iManufacturer = STRID_MANUFACTURER,
    .iProduct      = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,

    .bNumConfigurations = 1};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define CDC_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x82

static uint8_t const cdc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_CDC_NUM_TOTAL, 0, CDC_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_CDC_NUM_CDC, 4, EPNUM_CDC_NOTIF, 16, EPNUM_CDC_OUT, EPNUM_CDC_IN, CFG_TUD_CDC_EP_BUFSIZE),
};

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
static char const* string_desc_cdc_arr[] = {
    [STRID_LANGID]       = (const char[]) {0x09, 0x04},  // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "RHS",                        // Manufacturer
    [STRID_PRODUCT]      = "RHS Device",                 // Product
    [STRID_SERIAL]       = "123456",                     // Serial
    [STRID_INTERFACE]    = "RHS CDC Interface"           // Interface Description
};

static volatile bool          connected = false;
static volatile CdcCallbacks* callbacks = NULL;
static void*                  cb_ctx;
static uint8_t                cdc_ctrl_line_state;
static cdc_line_coding_t      line_coding;
static bool                   acquired;

void tud_cdc_rx_cb(uint8_t itf)
{
    UNUSED(itf);
    if (callbacks != NULL && callbacks->rx_callback != NULL)
    {
        callbacks->rx_callback(cb_ctx);
    }
}

void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char) {}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    UNUSED(itf);
    if (callbacks != NULL && callbacks->tx_callback != NULL)
    {
        callbacks->tx_callback(cb_ctx);
    }
}

void tud_cdc_notify_complete_cb(uint8_t itf)
{
    RHS_LOG_D(TAG, "Terminal %d notified", itf);
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    RHS_LOG_D(TAG, "Terminal %d DTR %d RTS %d", itf, dtr, rts);

    cdc_ctrl_line_state = (dtr ? CdcCtrlLineDTR : 0) | (rts ? CdcCtrlLineRTS : 0);
    connected           = (dtr || rts);

    if (callbacks != NULL)
    {
        if (callbacks->state_callback != NULL)
            callbacks->state_callback(cb_ctx, connected ? CdcStateConnected : CdcStateDisconnected);
        if (callbacks->ctrl_line_callback != NULL)
            callbacks->ctrl_line_callback(cb_ctx, (CdcCtrlLine) cdc_ctrl_line_state);
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding)
{
    RHS_LOG_D(TAG,
              "Terminal %d line coding changed %d %d %d %d",
              itf,
              p_line_coding->bit_rate,
              p_line_coding->data_bits,
              p_line_coding->parity,
              p_line_coding->stop_bits);

    line_coding = *p_line_coding;

    if (callbacks != NULL && callbacks->line_config_callback != NULL)
        callbacks->line_config_callback(cb_ctx, &line_coding);
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
    RHS_LOG_D(TAG, "Terminal %d send break %d ms", itf, duration_ms);
}

static int32_t usb_cdc_task(void* context);

static RHSThread* thread;
static bool       finish = false;

static void cdc_init(void* context)
{
    UNUSED(context);
    thread = rhs_thread_alloc("UsbCdc", 4096, usb_cdc_task, NULL);
    finish = false;
    rhs_thread_start(thread);
}

static void cdc_deinit(void* context)
{
    UNUSED(context);
    finish = true;
    // The task runs tusb_deinit() before it exits: USB must still be clocked, the dwc2 core reset (F7) waits for
    // AHB idle forever on a disabled peripheral
    rhs_thread_join(thread);
    rhs_thread_free(thread);
    rhs_hal_usb_disable();
}

RHSHalUsbInterface usb_cdc_desc = {
    .init                  = cdc_init,
    .deinit                = cdc_deinit,
    .device_desc           = &desc_device_cdc,
    .configuration_arr     = (uint8_t const* const[]) {cdc_configuration},
    .string_desc_arr       = (char const* const*) string_desc_cdc_arr,
    .string_desc_arr_count = TU_ARRAY_SIZE(string_desc_cdc_arr),
};

static int32_t usb_cdc_task(void* context)
{
    UNUSED(context);
    /* Reinit hardware USB (PINs and RESET USB) */
    rhs_hal_usb_reinit();
    /* Init TinyUSB */
    tusb_init();  // TODO check is not initialized

    while (!finish)
    {
        tud_task_ext(1, false);  // tinyusb device task
    }
    tusb_deinit(0);
    RHS_LOG_D(TAG, "USB CDC application finished");
    return 0;
}

void rhs_hal_cdc_set_callbacks(CdcCallbacks* cb, void* context)
{
    if (callbacks != NULL && callbacks->state_callback != NULL && connected == true)
    {
        callbacks->state_callback(cb_ctx, CdcStateDisconnected);
    }

    callbacks = cb;
    cb_ctx    = context;

    if (callbacks != NULL)
    {
        if (callbacks->state_callback != NULL && connected == true)
        {
            callbacks->state_callback(cb_ctx, CdcStateConnected);
        }
        if (callbacks->ctrl_line_callback != NULL)
        {
            callbacks->ctrl_line_callback(cb_ctx, cdc_ctrl_line_state);
        }
    }
}

bool rhs_hal_cdc_acquire(void)
{
    bool was_free;
    RHS_CRITICAL_ENTER();
    was_free = !acquired;
    acquired = true;
    RHS_CRITICAL_EXIT();

    if (was_free)
    {
        rhs_hal_usb_set_interface(&usb_cdc_desc, NULL);
    }
    return was_free;
}

void rhs_hal_cdc_release(void)
{
    rhs_hal_cdc_set_callbacks(NULL, NULL);
    if (rhs_hal_usb_get_interface() == &usb_cdc_desc)
    {
        rhs_hal_usb_set_interface(NULL, NULL);
    }
    acquired = false;
}

cdc_line_coding_t* rhs_hal_cdc_get_port_settings(void)
{
    return &line_coding;
}

uint8_t rhs_hal_cdc_get_ctrl_line_state(void)
{
    return cdc_ctrl_line_state;
}

uint16_t rhs_hal_cdc_send(const uint8_t* buf, uint16_t len)
{
    uint16_t written = (uint16_t) tud_cdc_n_write(CDC_ITF, buf, len);
    tud_cdc_n_write_flush(CDC_ITF);
    return written;
}

int32_t rhs_hal_cdc_receive(uint8_t* buf, uint16_t max_len)
{
    if (tud_cdc_n_available(CDC_ITF))
    {
        return tud_cdc_n_read(CDC_ITF, buf, max_len);
    }
    return 0;
}
