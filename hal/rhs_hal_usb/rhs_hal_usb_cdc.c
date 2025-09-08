#include "rhs.h"
#include "rhs_hal.h"
#include "rhs_hal_usb_cdc.h"
#include "cli.h"
#include <ctype.h>

#define TAG "UsbCDC"

#define IF_NUM_MAX 2

// Dual CDC configuration enums
enum
{
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_CDC_1,
    ITF_NUM_CDC_1_DATA,
    ITF_NUM_TOTAL_DUAL_CDC
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
    .idProduct = 0x5740,  // Different PID for dual CDC mode
    .bcdDevice = 0x0101,

    .iManufacturer = STRID_MANUFACTURER,
    .iProduct      = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,

    .bNumConfigurations = 1  // single configuration for dual CDC
};

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define DUAL_CDC_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

#define EPNUM_CDC_0_NOTIF 0x81
#define EPNUM_CDC_0_OUT 0x02
#define EPNUM_CDC_0_IN 0x82

#define EPNUM_CDC_1_NOTIF 0x83
#define EPNUM_CDC_1_OUT 0x04
#define EPNUM_CDC_1_IN 0x84

// Dual CDC configuration
static uint8_t const dual_cdc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL_DUAL_CDC, 0, DUAL_CDC_CONFIG_TOTAL_LEN, 0, 100),

    // 1st CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 16, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 32),

    // 2nd CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 4, EPNUM_CDC_1_NOTIF, 16, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 32),
};

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
static char const* string_desc_dual_cdc_arr[] = {
    [STRID_LANGID]       = (const char[]) {0x09, 0x04},  // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "TinyUSB",                    // Manufacturer
    [STRID_PRODUCT]      = "TinyUSB Device",             // Product
    [STRID_SERIAL]       = "123456",                     // Serial
    [STRID_INTERFACE]    = "TinyUSB CDC Interface"       // Interface Description

    // STRID_MAC index is handled separately
};

static RHSHalUsbInterface usb_cdc_desc = {
    .device_desc       = &desc_device_cdc,
    .configuration_arr = (uint8_t const* const[]) {dual_cdc_configuration},
    .string_desc_arr   = (char const* const*) string_desc_dual_cdc_arr,
};

static volatile bool          connected             = false;
static volatile CdcCallbacks* callbacks[IF_NUM_MAX] = {NULL};
static void*                  cb_ctx[IF_NUM_MAX];
static uint8_t                cdc_ctrl_line_state[IF_NUM_MAX];

void tud_cdc_rx_cb(uint8_t itf)
{
    if (callbacks[itf] != NULL)
    {
        if (callbacks[itf]->rx_callback != NULL)
        {
            callbacks[itf]->rx_callback(cb_ctx[itf]);
        }
    }
}

void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char) {}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
    if (callbacks[itf] != NULL)
    {
        if (callbacks[itf]->tx_callback != NULL)
        {
            callbacks[itf]->tx_callback(cb_ctx[itf]);
        }
    }
}

void tud_cdc_notify_complete_cb(uint8_t itf)
{
    RHS_LOG_D(TAG, "Terminal %d notified", itf);
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    RHS_LOG_D(TAG, "Terminal %d DTR %d RTS %d", itf, dtr, rts);
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
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
    RHS_LOG_D(TAG, "Terminal %d send break %d ms", itf, duration_ms);
}

int32_t usb_dual_cdc(void* context)
{
    /* Switch descriptor USB */
    rhs_hal_usb_set_interface(&usb_cdc_desc);

    /* Reinit hardware USB (PINs and RESET USB) */
    rhs_hal_usb_reinit();
    /* Init TinyUSB */
    tusb_init();

    while (1)
    {
        tud_task();  // tinyusb device task
    }
}

void rhs_hal_cdc_set_callbacks(uint8_t if_num, CdcCallbacks* cb, void* context)
{
    rhs_assert(if_num < IF_NUM_MAX);

    if (callbacks[if_num] != NULL)
    {
        if (callbacks[if_num]->state_callback != NULL)
        {
            if (connected == true)
                callbacks[if_num]->state_callback(cb_ctx[if_num], 0);
        }
    }

    callbacks[if_num] = cb;
    cb_ctx[if_num]    = context;

    if (callbacks[if_num] != NULL)
    {
        if (callbacks[if_num]->state_callback != NULL)
        {
            if (connected == true)
                callbacks[if_num]->state_callback(cb_ctx[if_num], 1);
        }
        if (callbacks[if_num]->ctrl_line_callback != NULL)
        {
            callbacks[if_num]->ctrl_line_callback(cb_ctx[if_num], cdc_ctrl_line_state[if_num]);
        }
    }
}

struct usb_cdc_line_coding* rhs_hal_cdc_get_port_settings(uint8_t if_num) {}

uint8_t rhs_hal_cdc_get_ctrl_line_state(uint8_t if_num) {}

void rhs_hal_cdc_send(uint8_t if_num, uint8_t* buf, uint16_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        tud_cdc_n_write_char(if_num, buf[i]);
    }
    tud_cdc_n_write_flush(if_num);
}

int32_t rhs_hal_cdc_receive(uint8_t if_num, uint8_t* buf, uint16_t max_len)
{
    if (tud_cdc_n_available(if_num))
    {
        return tud_cdc_n_read(if_num, buf, max_len);
    }
    return 0;
}
