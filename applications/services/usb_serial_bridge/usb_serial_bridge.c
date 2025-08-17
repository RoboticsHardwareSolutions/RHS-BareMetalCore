#include "rhs.h"
#include "rhs_hal.h"
#include "usb_serial_bridge.h"
#include "cli.h"
#include "tusb.h"
#include "hal.h"
#include <ctype.h>

#define TAG "UsbSerialBridge"

// echo to either Serial0 or Serial1
// with Serial0 as all lower case, Serial1 as all upper case
static void echo_serial_port(uint8_t itf, uint8_t buf[], uint32_t count) {
  uint8_t const case_diff = 'a' - 'A';

  for (uint32_t i = 0; i < count; i++) {
    if (itf == 0) {
      // echo back 1st port as lower case
      if (isupper(buf[i])) buf[i] += case_diff;
    } else {
      // echo back 2nd port as upper case
      if (islower(buf[i])) buf[i] -= case_diff;
    }

    tud_cdc_n_write_char(itf, buf[i]);
  }
  tud_cdc_n_write_flush(itf);
}

static void cdc_task(void)
{
    for (uint8_t itf = 0; itf < CFG_TUD_CDC; itf++)
    {
        // connected() check for DTR bit
        // Most but not all terminal client set this when making connection
        // if ( tud_cdc_n_connected(itf) )
        {
            if (tud_cdc_n_available(itf))
            {
                uint8_t  buf[64];
                uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

                // echo back to both serial ports
                echo_serial_port(itf, buf, count);
                // echo_serial_port(1, buf, count);
            }

            // An Event to send Uart status notification
            // tud_cdc_notify_uart_state(&uart_state);
        }
    }
}

static inline void usb_init()
{
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);
    rhs_delay_ms(40);                                   // Wait 4ms
    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;                  // Enable USB clock

    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(USB_LP_IRQn);
    NVIC_EnableIRQ(USB_HP_IRQn);
    // Note: STM32F103 doesn't have USB_OTG, it has USB FS device only
    // USB peripheral will be configured by TinyUSB
}

int32_t vcp_service(void* context)
{
    usb_init();
    tusb_init();

    while (1)
    {
        tud_task();  // tinyusb device task
        cdc_task();
    }
}
