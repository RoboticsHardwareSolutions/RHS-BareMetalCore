#pragma once
#include "stdbool.h"
#include "rhs.h"

#if defined(STM32F103xE)
#include <stm32f103xe.h>
#include  "stm32f1xx_ll_gpio.h"
#elif defined(STM32F407xx)
#    include <stm32f407xx.h>
#elif defined(STM32F765xx)
#    include <stm32f765xx.h>
#elif defined(STM32G0B1xx)
#    include <stm32g0b1xx.h>
#else
#    error "Device not specified for rhs_hal_gpio"
#endif

/**
 * Number of gpio on one port
 */
#define GPIO_NUMBER (16U)

/**
 * Interrupt callback prototype
 */
typedef void (*RHSGpioExtiCallback)(void* context);

/**
 * Gpio interrupt type
 */
typedef struct
{
    RHSGpioExtiCallback callback;
    void* context;
} RHSGpioInterrupt;

/**
 * Gpio modes
 */
typedef enum
{
    RHSGpioModeInput,
    RHSGpioModeOutputPushPull,
    RHSGpioModeOutputOpenDrain,
    RHSGpioModeAltFunctionPushPull,
    RHSGpioModeAltFunctionOpenDrain,
    RHSGpioModeAnalog,
    RHSGpioModeInterruptRise,
    RHSGpioModeInterruptFall,
    RHSGpioModeInterruptRiseFall,
    RHSGpioModeEventRise,
    RHSGpioModeEventFall,
    RHSGpioModeEventRiseFall,
} RHSGpioMode;

/**
 * Gpio pull modes
 */
typedef enum
{
    RHSGpioPullNo,
    RHSGpioPullUp,
    RHSGpioPullDown,
} RHSGpioPull;

/**
 * Gpio speed modes
 */
typedef enum
{
    RHSGpioSpeedLow,
    RHSGpioSpeedMedium,
    RHSGpioSpeedHigh,
    RHSGpioSpeedVeryHigh,
} RHSGpioSpeed;

/**
 * Gpio alternate functions
 */
typedef enum
{
    RHSGpioAltFnUnused,
    RHSGpioAltFnUSB
} RHSGpioAltFn;

/**
 * Gpio structure
 */
typedef struct
{
    GPIO_TypeDef* port;
    uint32_t pin;
} RHSGpioPin;

/**
 * GPIO initialization function, simple version
 * @param gpio  RHSGpioPin
 * @param mode  RHSGpioPin
 */
void rhs_hal_gpio_init_simple(const RHSGpioPin* gpio, const RHSGpioMode mode);

/**
 * GPIO initialization function, normal version
 * @param gpio  RHSGpioPin
 * @param mode  RHSGpioMode
 * @param pull  RHSGpioPull
 * @param speed RHSGpioSpeed
 */
void rhs_hal_gpio_init(const RHSGpioPin* gpio,
                       const RHSGpioMode mode,
                       const RHSGpioPull pull,
                       const RHSGpioSpeed speed);

/**
 * GPIO initialization function, extended version
 * @param gpio  RHSGpioPin
 * @param mode  RHSGpioMode
 * @param pull  RHSGpioPull
 * @param speed RHSGpioSpeed
 * @param alt_fn RHSGpioSpeed
 */
void rhs_hal_gpio_init_ex(const RHSGpioPin* gpio,
                          const RHSGpioMode mode,
                          const RHSGpioPull pull,
                          const RHSGpioSpeed speed,
                          const RHSGpioAltFn alt_fn);

/**
 * Add and enable interrupt
 * @param gpio RHSGpioPin
 * @param cb   RHSGpioExtiCallback
 * @param context  context for callback
 */
void rhs_hal_gpio_add_int_callback(const RHSGpioPin* gpio, RHSGpioExtiCallback cb, void* context);

/**
 * Enable interrupt
 * @param gpio RHSGpioPin
 */
void rhs_hal_gpio_enable_int_callback(const RHSGpioPin* gpio);

/**
 * Disable interrupt
 * @param gpio RHSGpioPin
 */
void rhs_hal_gpio_disable_int_callback(const RHSGpioPin* gpio);

/**
 * Remove interrupt
 * @param gpio RHSGpioPin
 */
void rhs_hal_gpio_remove_int_callback(const RHSGpioPin* gpio);

/**
 * GPIO write pin
 * @param gpio  RHSGpioPin
 * @param state true / false
 */
static inline void rhs_hal_gpio_write(const RHSGpioPin* gpio, const bool state)
{
    if (state == true)
    {
        LL_GPIO_SetOutputPin(gpio->port, gpio->pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(gpio->port, gpio->pin);
    }
}

/**
 * GPIO read pin
 * @param port GPIO port
 * @param pin pin mask
 * @return true / false
 */
static inline void rhs_hal_gpio_write_port_pin(GPIO_TypeDef* port, uint16_t pin, const bool state)
{
    if (state == true)
    {
        LL_GPIO_SetOutputPin(port, pin);
    }
    else
    {
        LL_GPIO_ResetOutputPin(port, pin);
    }
}

/**
 * GPIO read pin
 * @param gpio RHSGpioPin
 * @return true / false
 */
static inline bool rhs_hal_gpio_read(const RHSGpioPin* gpio)
{
    return LL_GPIO_IsOutputPinSet(gpio->port, gpio->pin);
}

/**
 * GPIO read pin
 * @param port GPIO port
 * @param pin pin mask
 * @return true / false
 */
static inline bool rhs_hal_gpio_read_port_pin(GPIO_TypeDef* port, uint16_t pin)
{
    return LL_GPIO_IsOutputPinSet(port, pin);
}
