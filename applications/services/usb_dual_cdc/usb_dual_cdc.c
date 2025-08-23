#include "rhs.h"
#include "rhs_hal.h"
#include "usb_dual_cdc.h"
#include "cli.h"
#include "tusb.h"
#include <ctype.h>

#define TAG "UsbCDC"

#define IF_NUM_MAX 2

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

void tud_cdc_notify_complete_cb(uint8_t itf) {}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;

    // TODO set some indicator
    if (tud_cdc_n_connected(itf))
    {
        RHS_LOG_D(TAG, "Terminal %d connected %d %d", itf, dtr, rts);
    }
    else
    {
        RHS_LOG_D(TAG, "Terminal %d disconnected %d %d", itf, dtr, rts);
    }
}

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding) {}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms) {}

void    descriptor_switch_mode(uint32_t new_mode);
int32_t usb_dual_cdc(void* context)
{
    /* Switch descriptor USB */
    descriptor_switch_mode(1);
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
