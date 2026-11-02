#include "rhs_hal_gpio.h"

#if defined(STM32F103xE)
#include "stm32f1xx_ll_exti.h"
#include "stm32f1xx_ll_gpio.h"
#elif defined(STM32F765xx)
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_exti.h"
#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_ll_system.h"
#elif  defined (STM32G0B1xx)
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_system.h"
#endif



static uint32_t rhs_hal_gpio_invalid_argument_crash(void)
{
    rhs_crash("Invalid argument");
    return 0;
}

#define GPIO_PORT_MAP(port, prefix)    \
    (((port) == (GPIOA))   ? prefix## A \
     : ((port) == (GPIOB)) ? prefix## B \
     : ((port) == (GPIOC)) ? prefix## C \
     : ((port) == (GPIOD)) ? prefix## D \
     : ((port) == (GPIOE)) ? prefix## E \
                           : rhs_hal_gpio_invalid_argument_crash())

#define GPIO_PIN_MAP(pin, prefix)               \
    (((pin) == (LL_GPIO_PIN_0))    ? prefix## 0  \
     : ((pin) == (LL_GPIO_PIN_1))  ? prefix## 1  \
     : ((pin) == (LL_GPIO_PIN_2))  ? prefix## 2  \
     : ((pin) == (LL_GPIO_PIN_3))  ? prefix## 3  \
     : ((pin) == (LL_GPIO_PIN_4))  ? prefix## 4  \
     : ((pin) == (LL_GPIO_PIN_5))  ? prefix## 5  \
     : ((pin) == (LL_GPIO_PIN_6))  ? prefix## 6  \
     : ((pin) == (LL_GPIO_PIN_7))  ? prefix## 7  \
     : ((pin) == (LL_GPIO_PIN_8))  ? prefix## 8  \
     : ((pin) == (LL_GPIO_PIN_9))  ? prefix## 9  \
     : ((pin) == (LL_GPIO_PIN_10)) ? prefix## 10 \
     : ((pin) == (LL_GPIO_PIN_11)) ? prefix## 11 \
     : ((pin) == (LL_GPIO_PIN_12)) ? prefix## 12 \
     : ((pin) == (LL_GPIO_PIN_13)) ? prefix## 13 \
     : ((pin) == (LL_GPIO_PIN_14)) ? prefix## 14 \
     : ((pin) == (LL_GPIO_PIN_15)) ? prefix## 15 \
                                   : rhs_hal_gpio_invalid_argument_crash())

#define GET_EXTI_LINE(pin) GPIO_PIN_MAP(pin, LL_EXTI_LINE_)

/* Port/line selector for the EXTI-source routing register (AFIO_EXTICR on
 * F1, SYSCFG_EXTICR on F7, EXTI_EXTICR on G0) -- a *different* constant set
 * than LL_GPIO_PIN_n, so it needs its own port/line maps rather than reusing
 * GPIO_PORT_MAP (which only covers A-E, insufficient for BMPLC_XL's IN pins on
 * GPIOF/G). */
#if defined(STM32F103xE)
#define EXTI_SRC_PORT_MAP(port) GPIO_PORT_MAP(port, LL_GPIO_AF_EXTI_PORT)
#define EXTI_SRC_LINE_MAP(pin) GPIO_PIN_MAP(pin, LL_GPIO_AF_EXTI_LINE)
#define rhs_hal_gpio_set_exti_source(port, pin) \
    LL_GPIO_AF_SetEXTISource(EXTI_SRC_PORT_MAP(port), EXTI_SRC_LINE_MAP(pin))
#elif defined(STM32F765xx)
#define EXTI_SRC_PORT_MAP(port)                 \
    (((port) == (GPIOA))   ? LL_SYSCFG_EXTI_PORTA \
     : ((port) == (GPIOB)) ? LL_SYSCFG_EXTI_PORTB \
     : ((port) == (GPIOC)) ? LL_SYSCFG_EXTI_PORTC \
     : ((port) == (GPIOD)) ? LL_SYSCFG_EXTI_PORTD \
     : ((port) == (GPIOE)) ? LL_SYSCFG_EXTI_PORTE \
     : ((port) == (GPIOF)) ? LL_SYSCFG_EXTI_PORTF \
     : ((port) == (GPIOG)) ? LL_SYSCFG_EXTI_PORTG \
     : ((port) == (GPIOH)) ? LL_SYSCFG_EXTI_PORTH \
                            : rhs_hal_gpio_invalid_argument_crash())
#define EXTI_SRC_LINE_MAP(pin) GPIO_PIN_MAP(pin, LL_SYSCFG_EXTI_LINE)
#define rhs_hal_gpio_set_exti_source(port, pin) \
    LL_SYSCFG_SetEXTISource(EXTI_SRC_PORT_MAP(port), EXTI_SRC_LINE_MAP(pin))
#elif defined(STM32G0B1xx)
/* Map GPIO port to EXTI config port (LL_EXTI_CONFIG_PORTx) */
#define EXTI_SRC_PORT_MAP(port)                 \
    (((port) == (GPIOA))   ? LL_EXTI_CONFIG_PORTA \
     : ((port) == (GPIOB)) ? LL_EXTI_CONFIG_PORTB \
     : ((port) == (GPIOC)) ? LL_EXTI_CONFIG_PORTC \
     : ((port) == (GPIOD)) ? LL_EXTI_CONFIG_PORTD \
     : ((port) == (GPIOE)) ? LL_EXTI_CONFIG_PORTE \
     : ((port) == (GPIOF)) ? LL_EXTI_CONFIG_PORTF \
                            : rhs_hal_gpio_invalid_argument_crash())
/* Map pin to EXTI config line (LL_EXTI_CONFIG_LINEy) */
#define EXTI_SRC_LINE_MAP(pin)                  \
    (((pin) == (LL_GPIO_PIN_0))    ? LL_EXTI_CONFIG_LINE0  \
     : ((pin) == (LL_GPIO_PIN_1))  ? LL_EXTI_CONFIG_LINE1  \
     : ((pin) == (LL_GPIO_PIN_2))  ? LL_EXTI_CONFIG_LINE2  \
     : ((pin) == (LL_GPIO_PIN_3))  ? LL_EXTI_CONFIG_LINE3  \
     : ((pin) == (LL_GPIO_PIN_4))  ? LL_EXTI_CONFIG_LINE4  \
     : ((pin) == (LL_GPIO_PIN_5))  ? LL_EXTI_CONFIG_LINE5  \
     : ((pin) == (LL_GPIO_PIN_6))  ? LL_EXTI_CONFIG_LINE6  \
     : ((pin) == (LL_GPIO_PIN_7))  ? LL_EXTI_CONFIG_LINE7  \
     : ((pin) == (LL_GPIO_PIN_8))  ? LL_EXTI_CONFIG_LINE8  \
     : ((pin) == (LL_GPIO_PIN_9))  ? LL_EXTI_CONFIG_LINE9  \
     : ((pin) == (LL_GPIO_PIN_10)) ? LL_EXTI_CONFIG_LINE10 \
     : ((pin) == (LL_GPIO_PIN_11)) ? LL_EXTI_CONFIG_LINE11 \
     : ((pin) == (LL_GPIO_PIN_12)) ? LL_EXTI_CONFIG_LINE12 \
     : ((pin) == (LL_GPIO_PIN_13)) ? LL_EXTI_CONFIG_LINE13 \
     : ((pin) == (LL_GPIO_PIN_14)) ? LL_EXTI_CONFIG_LINE14 \
     : ((pin) == (LL_GPIO_PIN_15)) ? LL_EXTI_CONFIG_LINE15 \
                                   : rhs_hal_gpio_invalid_argument_crash())

#define rhs_hal_gpio_set_exti_source(port, pin) \
    LL_EXTI_SetEXTISource(EXTI_SRC_PORT_MAP(port), EXTI_SRC_LINE_MAP(pin))
#endif

static volatile RHSGpioInterrupt gpio_interrupt[GPIO_NUMBER];

#if defined(STM32F765xx) || defined (STM32F103xE)
static IRQn_Type rhs_hal_gpio_exti_irqn(uint8_t pin_num)
{
    switch (pin_num)
    {
    case 0:
        return EXTI0_IRQn;
    case 1:
        return EXTI1_IRQn;
    case 2:
        return EXTI2_IRQn;
    case 3:
        return EXTI3_IRQn;
    case 4:
        return EXTI4_IRQn;
    default:
        return (pin_num <= 9) ? EXTI9_5_IRQn : EXTI15_10_IRQn;
    }
}
#elif defined (STM32G0B1xx)
static IRQn_Type rhs_hal_gpio_exti_irqn(uint8_t pin_num)
{
    switch (pin_num)
    {
    case 0:
    case 1:
        return EXTI0_1_IRQn;
    case 2:
    case 3:
        return EXTI2_3_IRQn;
    default:
        return EXTI4_15_IRQn;
    }
}
#endif

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

/* Macros to abstract platform differences for EXTI flag handling */
#if defined(STM32G0B1xx)
/* STM32G0 uses separate rising/falling flag registers (RPR1/FPR1).
 * Writing 1 clears the flag. Use direct register writes for simplicity. */
#define RHS_EXTI_CLEAR_FLAG(line)    \
    do {                             \
        EXTI->RPR1 = (line);         \
        EXTI->FPR1 = (line);         \
    } while (0)
#define RHS_EXTI_IS_ACTIVE_FLAG(line) ((EXTI->RPR1 & (line)) || (EXTI->FPR1 & (line)))
#else
/* STM32F1/STM32F7 use LL_EXTI_ClearFlag_0_31 and LL_EXTI_IsActiveFlag_0_31 */
#define RHS_EXTI_CLEAR_FLAG(line)    LL_EXTI_ClearFlag_0_31(line)
#define RHS_EXTI_IS_ACTIVE_FLAG(line) LL_EXTI_IsActiveFlag_0_31(line)
#endif

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
#if defined(STM32F765xx)
        // SYSCFG (the EXTI-source router on F7) has no other enabler in this
        // codebase -- idempotent, safe to call on every interrupt-mode config.
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
#endif
        // Route this pin's port onto its EXTI line (AFIO_EXTICR on F1,
        // SYSCFG_EXTICR on F7) -- without this, the EXTI line stays routed
        // to whatever port reset selected (Port A), so a pin on any other
        // port would silently never see its own edges.
        rhs_hal_gpio_set_exti_source(gpio->port, gpio->pin);

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
            RHS_EXTI_CLEAR_FLAG(exti_line);
            LL_EXTI_DisableRisingTrig_0_31(exti_line);
            LL_EXTI_DisableFallingTrig_0_31(exti_line);
        }

        // Set not interrupt pin modes
        switch (mode)
        {
        case RHSGpioModeInput:
#if defined(STM32F103xE)
            /* On F1, CNF/MODE are packed into one field: LL_GPIO_MODE_INPUT actually
             * selects "input with pull" (CNF=10), and LL_GPIO_MODE_FLOATING selects
             * floating input (CNF=01) -- they are alternatives, not combinable. Pick
             * one directly instead of ORing LL_GPIO_MODE_FLOATING's bit on top of
             * LL_GPIO_MODE_INPUT's (which produces the reserved CNF=11 encoding). */
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, pull == RHSGpioPullNo ? LL_GPIO_MODE_FLOATING : LL_GPIO_MODE_INPUT);
#else
            LL_GPIO_SetPinMode(gpio->port, gpio->pin, LL_GPIO_MODE_INPUT);
            if (pull == RHSGpioPullNo)
            {
                LL_GPIO_SetPinPull(gpio->port, gpio->pin, LL_GPIO_PULL_NO);
            }
#endif
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

    // LL_EXTI_EnableIT_0_31() only unmasks the line at the EXTI peripheral --
    // the NVIC vector for it is never enabled anywhere else in this codebase
    // (unlike CAN/UART/etc, which go through rhs_hal_interrupt.c's own IRQn
    // table), so without this the interrupt would latch but never dispatch.
    IRQn_Type irqn = rhs_hal_gpio_exti_irqn(pin_num);
    NVIC_SetPriority(irqn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(irqn);

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
    RHS_EXTI_CLEAR_FLAG(exti_line);

    RHS_CRITICAL_EXIT();
}

void rhs_hal_gpio_remove_int_callback(const RHSGpioPin* gpio)
{
    rhs_assert(gpio);

    RHS_CRITICAL_ENTER();

    const uint32_t exti_line = GET_EXTI_LINE(gpio->pin);
    LL_EXTI_DisableIT_0_31(exti_line);
    RHS_EXTI_CLEAR_FLAG(exti_line);

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

/* Helper macro for interrupt handlers - checks and clears flag for a specific line */
#if defined(STM32G0B1xx)
/* For STM32G0, check both rising and falling flag registers */
#define RHS_EXTI_HANDLER(line) \
    if (RHS_EXTI_IS_ACTIVE_FLAG(LL_EXTI_LINE_##line)) { \
        RHS_EXTI_CLEAR_FLAG(LL_EXTI_LINE_##line); \
        rhs_hal_gpio_int_call(line); \
    }
#else
/* For STM32F1/STM32F7, use the standard functions */
#define RHS_EXTI_HANDLER(line) \
    if (RHS_EXTI_IS_ACTIVE_FLAG(LL_EXTI_LINE_##line)) { \
        RHS_EXTI_CLEAR_FLAG(LL_EXTI_LINE_##line); \
        rhs_hal_gpio_int_call(line); \
    }
#endif

/* Interrupt handlers for STM32F1/STM32F7 style individual EXTI lines */
void EXTI0_IRQHandler(void)
{
    RHS_EXTI_HANDLER(0);
}

void EXTI1_IRQHandler(void)
{
    RHS_EXTI_HANDLER(1);
}

void EXTI2_IRQHandler(void)
{
    RHS_EXTI_HANDLER(2);
}

void EXTI3_IRQHandler(void)
{
    RHS_EXTI_HANDLER(3);
}

void EXTI4_IRQHandler(void)
{
    RHS_EXTI_HANDLER(4);
}

void EXTI9_5_IRQHandler(void)
{
    RHS_EXTI_HANDLER(5);
    RHS_EXTI_HANDLER(6);
    RHS_EXTI_HANDLER(7);
    RHS_EXTI_HANDLER(8);
    RHS_EXTI_HANDLER(9);
}

void EXTI15_10_IRQHandler(void)
{
    RHS_EXTI_HANDLER(10);
    RHS_EXTI_HANDLER(11);
    RHS_EXTI_HANDLER(12);
    RHS_EXTI_HANDLER(13);
    RHS_EXTI_HANDLER(14);
    RHS_EXTI_HANDLER(15);
}

/* Interrupt handlers for STM32G0 (combined EXTI lines) */
#if defined(STM32G0B1xx)
void EXTI0_1_IRQHandler(void)
{
    RHS_EXTI_HANDLER(0);
    RHS_EXTI_HANDLER(1);
}

void EXTI2_3_IRQHandler(void)
{
    RHS_EXTI_HANDLER(2);
    RHS_EXTI_HANDLER(3);
}

void EXTI4_15_IRQHandler(void)
{
    RHS_EXTI_HANDLER(4);
    RHS_EXTI_HANDLER(5);
    RHS_EXTI_HANDLER(6);
    RHS_EXTI_HANDLER(7);
    RHS_EXTI_HANDLER(8);
    RHS_EXTI_HANDLER(9);
    RHS_EXTI_HANDLER(10);
    RHS_EXTI_HANDLER(11);
    RHS_EXTI_HANDLER(12);
    RHS_EXTI_HANDLER(13);
    RHS_EXTI_HANDLER(14);
    RHS_EXTI_HANDLER(15);
}
#endif
