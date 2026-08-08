#include "rhs_hal_gpio.h"
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_gpio.h"

static uint32_t furi_hal_gpio_invalid_argument_crash(void)
{
    rhs_crash("Invalid argument");
    return 0;
}

#define GPIO_PORT_MAP(port, prefix)    \
    (((port) == (GPIOA))   ? prefix##A \
     : ((port) == (GPIOB)) ? prefix##B \
     : ((port) == (GPIOC)) ? prefix##C \
     : ((port) == (GPIOD)) ? prefix##D \
     : ((port) == (GPIOE)) ? prefix##E \
                           : furi_hal_gpio_invalid_argument_crash())

#define GPIO_PIN_MAP(pin, prefix)               \
    (((pin) == (LL_GPIO_PIN_0))    ? prefix##0  \
     : ((pin) == (LL_GPIO_PIN_1))  ? prefix##1  \
     : ((pin) == (LL_GPIO_PIN_2))  ? prefix##2  \
     : ((pin) == (LL_GPIO_PIN_3))  ? prefix##3  \
     : ((pin) == (LL_GPIO_PIN_4))  ? prefix##4  \
     : ((pin) == (LL_GPIO_PIN_5))  ? prefix##5  \
     : ((pin) == (LL_GPIO_PIN_6))  ? prefix##6  \
     : ((pin) == (LL_GPIO_PIN_7))  ? prefix##7  \
     : ((pin) == (LL_GPIO_PIN_8))  ? prefix##8  \
     : ((pin) == (LL_GPIO_PIN_9))  ? prefix##9  \
     : ((pin) == (LL_GPIO_PIN_10)) ? prefix##10 \
     : ((pin) == (LL_GPIO_PIN_11)) ? prefix##11 \
     : ((pin) == (LL_GPIO_PIN_12)) ? prefix##12 \
     : ((pin) == (LL_GPIO_PIN_13)) ? prefix##13 \
     : ((pin) == (LL_GPIO_PIN_14)) ? prefix##14 \
     : ((pin) == (LL_GPIO_PIN_15)) ? prefix##15 \
                                   : furi_hal_gpio_invalid_argument_crash())

// #define GET_SYSCFG_EXTI_PORT(port) GPIO_PORT_MAP(port, LL_SYSCFG_EXTI_PORT)
#define GET_SYSCFG_EXTI_LINE(pin) GPIO_PIN_MAP(pin, LL_SYSCFG_EXTI_LINE)
#define GET_EXTI_LINE(pin) GPIO_PIN_MAP(pin, LL_EXTI_LINE_)

// #define GET_PWR_PORT(port) GPIO_PORT_MAP(port, LL_PWR_GPIO_)
// #define GET_PWR_PIN(pin) GPIO_PIN_MAP(pin, LL_PWR_GPIO_BIT_)

static volatile RHSGpioInterrupt gpio_interrupt[GPIO_NUMBER];

static uint8_t rhs_hal_gpio_get_pin_num(const RHSGpioPin* gpio)
{
    uint8_t pin_num = 0;
    for (pin_num = 0; pin_num < GPIO_NUMBER; pin_num++)
    {
        if (gpio->pin & (1 << pin_num))
            break;
    }
    return pin_num;
}

void rhs_hal_gpio_init_simple(const RHSGpioPin* gpio, const RHSGpioMode mode)
{
    rhs_hal_gpio_init(gpio, mode, RHSGpioPullNo, RHSGpioSpeedLow);
}

void rhs_hal_gpio_init(const RHSGpioPin* gpio, const RHSGpioMode mode, const RHSGpioPull pull, const RHSGpioSpeed speed)
{
    // we cannot set alternate mode in this function
    rhs_assert(mode != RHSGpioModeAltFunctionPushPull);
    rhs_assert(mode != RHSGpioModeAltFunctionOpenDrain);

    rhs_hal_gpio_init_ex(gpio, mode, pull, speed, RHSGpioAltFnUnused);
}

void rhs_hal_gpio_init_ex(const RHSGpioPin* gpio,
                          const RHSGpioMode mode,
                          const RHSGpioPull pull,
                          const RHSGpioSpeed speed,
                          const RHSGpioAltFn alt_fn)
{
    // TODO add  rhs_assert for unavaliable alt_fn or rhs_crash

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);

    // Configure gpio with interrupts disabled
    RHS_CRITICAL_ENTER();

    // Set gpio speed
    switch (speed)
    {
    case RHSGpioSpeedLow:
        LL_GPIO_SetPinSpeed(gpio->port, gpio->pin, LL_GPIO_SPEED_FREQ_LOW);
        break;
    case RHSGpioSpeedMedium:
        LL_GPIO_SetPinSpeed(gpio->port, gpio->pin, LL_GPIO_SPEED_FREQ_MEDIUM);
        break;
    case RHSGpioSpeedHigh:
        LL_GPIO_SetPinSpeed(gpio->port, gpio->pin, LL_GPIO_SPEED_FREQ_HIGH);
        break;
    case RHSGpioSpeedVeryHigh:
        rhs_crash("Gpio speed not supported");
        break;
    }


    // Set gpio mode
    if (mode >= RHSGpioModeInterruptRise)
    {
        // Set pin in interrupt mode
        LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_INPUT);
        if (mode == RHSGpioModeInterruptRise || mode == RHSGpioModeInterruptRiseFall)
        {
            LL_EXTI_EnableRisingTrig_0_31(exti_line);
        }
        if (mode == RHSGpioModeInterruptFall || mode == RHSGpioModeInterruptRiseFall)
        {
            LL_EXTI_EnableFallingTrig_0_31(exti_line);
        }
        if (mode == RHSGpioModeEventRise || mode == RHSGpioModeEventRiseFall)
        {
            LL_EXTI_EnableEvent_0_31(exti_line);
            LL_EXTI_EnableRisingTrig_0_31(exti_line);
        }
        if (mode == RHSGpioModeEventFall || mode == RHSGpioModeEventRiseFall)
        {
            LL_EXTI_EnableEvent_0_31(exti_line);
            LL_EXTI_EnableFallingTrig_0_31(exti_line);
        }
    }
    else
    {
        // Disable interrupts
        {
            LL_EXTI_DisableIT_0_31(exti_line);
            LL_EXTI_ClearFlag_0_31(exti_line);
            LL_EXTI_DisableRisingTrig_0_31(exti_line);
            LL_EXTI_DisableFallingTrig_0_31(exti_line);
        }

        // Set not interrupt pin modes
        switch (mode)
        {
        case RHSGpioModeInput:
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_INPUT);
            if (pull == RHSGpioPullNo)
                LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_MODE_FLOATING);
            break;
        case RHSGpioModeOutputPushPull:
            LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_PUSHPULL);
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_OUTPUT);
            break;
        case RHSGpioModeAltFunctionPushPull:
            LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_PUSHPULL);
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_ALTERNATE);
            break;
        case RHSGpioModeOutputOpenDrain:
            LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_OPENDRAIN);
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_OUTPUT);
            break;
        case RHSGpioModeAltFunctionOpenDrain:
            LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_OPENDRAIN);
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_ALTERNATE);
            break;
        case RHSGpioModeAnalog:
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_ANALOG);
            break;
        default:
            rhs_crash("Incorrect GpioMode");
        }
    }

    // Set gpio pull mode
    switch (pull)
    {
    case RHSGpioPullUp:
        LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinPull(gpio->port, gpio->pin,LL_GPIO_PULL_UP);
        break;
    case RHSGpioPullDown:
        LL_GPIO_SetPinOutputType(gpio->port, gpio->pin, LL_GPIO_OUTPUT_PUSHPULL);
        LL_GPIO_SetPinPull(gpio->port, gpio->pin,LL_GPIO_PULL_DOWN);
        break;
    case RHSGpioPullNo:
        break;
    default:
        rhs_crash("Incorrect GpioPull");
    }


    // TODO add remap for BMPLC M !!!
    RHS_CRITICAL_EXIT();
}

void rhs_hal_gpio_add_int_callback(const RHSGpioPin* gpio, RHSGpioExtiCallback cb, void* context)
{
    rhs_assert(gpio);
    rhs_assert(cb);

    RHS_CRITICAL_ENTER();

    uint8_t pin_num = rhs_hal_gpio_get_pin_num(gpio);
    rhs_assert(gpio_interrupt[pin_num].callback == NULL);
    gpio_interrupt[pin_num].callback = cb;
    gpio_interrupt[pin_num].context = context;

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);
    LL_EXTI_EnableIT_0_31(exti_line);

    RHS_CRITICAL_EXIT();
}

void rhs_hal_gpio_enable_int_callback(const RHSGpioPin* gpio)
{
    rhs_assert(gpio);

    RHS_CRITICAL_ENTER();

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);
    LL_EXTI_EnableIT_0_31(exti_line);

    RHS_CRITICAL_EXIT();
}

void rhs_hal_gpio_disable_int_callback(const RHSGpioPin* gpio)
{
    rhs_assert(gpio);

    RHS_CRITICAL_ENTER();

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);
    LL_EXTI_DisableIT_0_31(exti_line);
    LL_EXTI_ClearFlag_0_31(exti_line);

    RHS_CRITICAL_EXIT();
}

void rhs_hal_gpio_remove_int_callback(const RHSGpioPin* gpio)
{
    rhs_assert(gpio);

    RHS_CRITICAL_ENTER();

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);
    LL_EXTI_DisableIT_0_31(exti_line);
    LL_EXTI_ClearFlag_0_31(exti_line);

    uint8_t pin_num = rhs_hal_gpio_get_pin_num(gpio);
    gpio_interrupt[pin_num].callback = NULL;
    gpio_interrupt[pin_num].context = NULL;

    RHS_CRITICAL_EXIT();
}

inline static void rhs_hal_gpio_int_call(uint16_t pin_num)
{
    if (gpio_interrupt[pin_num].callback)
    {
        gpio_interrupt[pin_num].callback(gpio_interrupt[pin_num].context);
    }
}

/* Interrupt handlers */
void EXTI0_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
        rhs_hal_gpio_int_call(0);
    }
}

void EXTI1_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_1))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_1);
        rhs_hal_gpio_int_call(1);
    }
}

void EXTI2_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_2))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_2);
        rhs_hal_gpio_int_call(2);
    }
}

void EXTI3_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_3))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_3);
        rhs_hal_gpio_int_call(3);
    }
}

void EXTI4_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
        rhs_hal_gpio_int_call(4);
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_5))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_5);
        rhs_hal_gpio_int_call(5);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
        rhs_hal_gpio_int_call(6);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_7))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
        rhs_hal_gpio_int_call(7);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_8))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_8);
        rhs_hal_gpio_int_call(8);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_9))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_9);
        rhs_hal_gpio_int_call(9);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_10))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_10);
        rhs_hal_gpio_int_call(10);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_11))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);
        rhs_hal_gpio_int_call(11);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);
        rhs_hal_gpio_int_call(12);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_13))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
        rhs_hal_gpio_int_call(13);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_14))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_14);
        rhs_hal_gpio_int_call(14);
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_15))
    {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_15);
        rhs_hal_gpio_int_call(15);
    }
}
