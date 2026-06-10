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

#elif defined(STM32G0B1xx)
    // Enable HSI48 for USB
    RCC->CR |= RCC_CR_HSI48ON;
    while (!(RCC->CR & RCC_CR_HSI48RDY))
        ;  // Wait for HSI48 ready

    // Select HSI48 as USB clock source (CCIPR2.USBSEL = 00b)
    RCC->CCIPR2 &= ~RCC_CCIPR2_USBSEL;

    // Enable VDDUSB (USB voltage detector)
    RCC->APBENR1 |= RCC_APBENR1_PWREN;  // Enable PWR clock if not already
    PWR->CR2 |= PWR_CR2_USV;            // USB supply valid

    // Disable internal pull-up in dead battery pins (UCPD strobe)
    RCC->APBENR2 |= RCC_APBENR2_SYSCFGEN;  // Enable SYSCFG clock
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_UCPD1_STROBE;

    // Reset and disable USB peripheral
    RCC->APBRSTR1 |= RCC_APBRSTR1_USBRST;
    RCC->APBENR1 &= ~RCC_APBENR1_USBEN;

    // Configure USB pins (D-/D+) as output and pull low for USB reset
    gpio_init(PIN('A', 11), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    rhs_delay_ms(40);

    // Enable and release USB reset
    RCC->APBENR1 |= RCC_APBENR1_USBEN;
    RCC->APBRSTR1 &= ~RCC_APBRSTR1_USBRST;

    // Configure as floating input - USB peripheral will control these pins
    gpio_init(PIN('A', 11), MG_GPIO_MODE_INPUT, 0, 0, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_INPUT, 0, 0, MG_GPIO_PULL_NONE, 0);  // D+

    NVIC_SetPriority(USB_UCPD1_2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

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

#elif defined(STM32G0B1xx)
    RCC->APBRSTR1 |= RCC_APBRSTR1_USBRST;
    RCC->APBENR1 &= ~RCC_APBENR1_USBEN;
    gpio_init(PIN('A', 11), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    rhs_delay_ms(40);
    RCC->APBENR1 |= RCC_APBENR1_USBEN;
    RCC->APBRSTR1 &= ~RCC_APBRSTR1_USBRST;

    // Configure as floating input - USB peripheral will control these pins
    gpio_init(PIN('A', 11), MG_GPIO_MODE_INPUT, 0, 0, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_INPUT, 0, 0, MG_GPIO_PULL_NONE, 0);  // D+

    NVIC_SetPriority(USB_UCPD1_2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(USB_UCPD1_2_IRQn);

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

#elif defined(STM32G0B1xx)
    RCC->APBRSTR1 |= RCC_APBRSTR1_USBRST;
    RCC->APBENR1 &= ~RCC_APBENR1_USBEN;
    gpio_init(PIN('A', 11), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D-
    gpio_init(PIN('A', 12), MG_GPIO_MODE_OUTPUT, MG_GPIO_OTYPE_PUSH_PULL, MG_GPIO_SPEED_HIGH, MG_GPIO_PULL_NONE, 0);  // D+

    gpio_write(PIN('A', 11), 0);
    gpio_write(PIN('A', 12), 0);

    NVIC_DisableIRQ(USB_UCPD1_2_IRQn);

#else
#    error "Unknown platform"
#endif
}

extern void descriptor_switch_mode(tusb_desc_device_t* new_desc,
                                   uint8_t const**     new_config,
                                   char const**        new_string_desc_arr,
                                   size_t              new_string_desc_arr_count);

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
                               (char const**) iface->string_desc_arr,
                               iface->string_desc_arr_count);

        if (iface->init)
            iface->init();
    }
    s_usb_desc = iface;
}

RHSHalUsbInterface* rhs_hal_usb_get_interface(void)
{
    return s_usb_desc;
}
