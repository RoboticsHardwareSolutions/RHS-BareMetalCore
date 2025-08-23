#include "rhs.h"
#include "rhs_hal.h"
#include "usb_dual_cdc.h"
#include "cli.h"
#include "tusb.h"
#include "hal.h"
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

// static void cdc_task(void)
// {
//     for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++)
//     {
//         // connected() check for DTR bit
//         // Most but not all terminal client set this when making connection
//         // if ( tud_cdc_n_connected(itf) )
//         {
//             if (tud_cdc_n_available(itf))
//             {
//                 uint8_t  buf[64];
//                 uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

//                 // echo back to both serial ports
//                 echo_serial_port(itf, buf, count);
//                 // echo_serial_port(1, buf, count);
//             }

//             // An Event to send Uart status notification
//             // tud_cdc_notify_uart_state(&uart_state);
//         }
//     }
// }

static inline void usb_init(void)
{
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    rhs_delay_ms(40);  // Wait 40ms
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USBRST;

    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;                  // Enable USB clock

    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    // Note: STM32F103 doesn't have USB_OTG, it has USB FS device only
    // USB peripheral will be configured by TinyUSB
}

void    descriptor_switch_mode(uint32_t new_mode);
int32_t usb_dual_cdc(void* context)
{
    /* Switch descriptor USB */
    descriptor_switch_mode(1);
    /* Reinit hardware USB (PINs and RESET USB) */
    usb_init();
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
