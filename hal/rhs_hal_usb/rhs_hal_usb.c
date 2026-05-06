#include <rhs.h>
#include <rhs_hal.h>

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
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    gpio_init(PIN('A', 11), MG_GPIO_MODE_AF, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 10);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_AF, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 10);  // D+
    NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(OTG_FS_IRQn);
#else
#    error "Unknown platform"
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
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    gpio_init(PIN('A', 11), MG_GPIO_MODE_AF, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 10);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_AF, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 10);  // D+
    NVIC_SetPriority(OTG_FS_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(OTG_FS_IRQn);
#else
#    error "Unknown platform"
#endif
    RHS_LOG_I(TAG, "Reinit OK");
}

void rhs_hal_usb_disable(void)
{
#if defined(STM32F1)
    RCC->APB1RSTR |= RCC_APB1RSTR_USBRST;
    RCC->APB1ENR &= ~RCC_APB1ENR_USBEN;
    gpio_init(PIN('A', 11), GPIO_MODE_OUTPUT_PP_50MHZ);  // D-
    gpio_init(PIN('A', 12), GPIO_MODE_OUTPUT_PP_50MHZ);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

#elif defined(STM32F4)
    RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;
    gpio_init(PIN('A', 11), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_INSANE, MG_GPIO_PULL_NONE, 0);  // D+
    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);
    NVIC_DisableIRQ(OTG_FS_IRQn);
#else
#    error "Unknown platform"
#endif
}

extern void descriptor_switch_mode(tusb_desc_device_t* new_desc,
                                   uint8_t const**     new_config,
                                   char const**        new_string_desc_arr);

static RHSHalUsbInterface* s_usb_desc = NULL;

void rhs_hal_usb_set_interface(RHSHalUsbInterface* iface)
{
    if (iface == s_usb_desc)
        return;
    if (s_usb_desc != NULL)
    {
        if (s_usb_desc->deinit)
            s_usb_desc->deinit();
    }
    if (iface != NULL)
    {
        // TODO init deinit interface
        // TODO chech if iface different from current
        descriptor_switch_mode((tusb_desc_device_t*) iface->device_desc,
                               (uint8_t const**) iface->configuration_arr,
                               (char const**) iface->string_desc_arr);

        if (iface->init)
            iface->init();
    }
    s_usb_desc = iface;
}

RHSHalUsbInterface* rhs_hal_usb_get_interface(void)
{
    return s_usb_desc;
}
