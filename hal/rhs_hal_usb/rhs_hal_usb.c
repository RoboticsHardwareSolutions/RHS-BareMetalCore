#include <rhs_hal_version.h>
#include <rhs_hal_usb.h>
#include <rhs_hal_power.h>
#include <rhs.h>
#include "hal.h"

#include "stm32.h"

#define TAG "RHSHalUsb"

/* Low-level init */
void rhs_hal_usb_init(void)
{
#if defined(STM32F1)
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    rhs_delay_ms(40);  // Wait 4ms
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USBRST;

    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+

    NVIC_SetPriority(USB_LP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 5, 0));
    NVIC_SetPriority(USB_HP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_SetPriority(OTG_FS_WKUP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

#elif defined(STM32F4)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(OTG_FS_IRQn);
#endif
    RHS_LOG_I(TAG, "Init OK");
}

void rhs_hal_usb_reinit(void)
{
#if defined(STM32F1)
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    rhs_delay_ms(40);  // Wait 40ms
    RCC->APB1ENR |= RCC_APB1ENR_USBEN;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USBRST;

    gpio_init(PIN('A', 11), GPIO_MODE_INPUT_FLOATING);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_INPUT_FLOATING);  // D+

#elif defined(STM32F4)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(OTG_FS_IRQn);
#endif
    RHS_LOG_I(TAG, "Reinit OK");
}

extern void descriptor_switch_mode(tusb_desc_device_t* new_desc,
                                   uint8_t const**     new_config,
                                   char const**        new_string_desc_arr);

void rhs_hal_usb_set_interface(RHSHalUsbInterface* iface)
{
    rhs_assert(iface);
    rhs_assert(iface->device_desc);
    rhs_assert(iface->configuration_arr);
    rhs_assert(iface->string_desc_arr);

    descriptor_switch_mode((tusb_desc_device_t*) iface->device_desc,
                           (uint8_t const**) iface->configuration_arr,
                           (char const**) iface->string_desc_arr);
}
