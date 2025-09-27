#include "rhs.h"
#include "rhs_hal_i2c_type.h"

#if defined(STM32F103xE)
#    include <stm32f1xx_ll_bus.h>
#    include <stm32f1xx_ll_gpio.h>
#elif defined(STM32F407xx)
#    include <stm32f4xx_ll_bus.h>
#    include <stm32f4xx_ll_gpio.h>
#elif defined(STM32F765xx)
#    include <stm32f7xx_ll_bus.h>
#    include <stm32f7xx_ll_gpio.h>
#else
#    error "Unsupported platform"
#endif

RHSMutex* rhs_hal_i2c_bus_external_mutex = NULL;

static void rhs_hal_i2c_bus_external_event(RHSHalI2cBus* bus, RHSHalI2cBusEvent event)
{
    if (event == RHSHalI2cBusEventInit)
    {
        rhs_hal_i2c_bus_external_mutex = rhs_mutex_alloc(RHSMutexTypeNormal);
        bus->current_handle            = NULL;
    }
    else if (event == RHSHalI2cBusEventDeinit)
    {
        rhs_mutex_free(rhs_hal_i2c_bus_external_mutex);
    }
    else if (event == RHSHalI2cBusEventLock)
    {
        rhs_assert(rhs_mutex_acquire(rhs_hal_i2c_bus_external_mutex, RHSWaitForever) == RHSStatusOk);
    }
    else if (event == RHSHalI2cBusEventUnlock)
    {
        rhs_assert(rhs_mutex_release(rhs_hal_i2c_bus_external_mutex) == RHSStatusOk);
    }
    else if (event == RHSHalI2cBusEventActivate)
    {
        RHS_CRITICAL_ENTER();
#if defined(STM32F103xE) || defined(STM32F407xx)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
#elif defined(STM32F765xx)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
        LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
#endif
        RHS_CRITICAL_EXIT();
    }
    else if (event == RHSHalI2cBusEventDeactivate)
    {
#if defined(STM32F103xE) || defined(STM32F407xx)
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
#elif defined(STM32F765xx)
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
#endif
    }
}

static RHSHalI2cBus rhs_hal_i2c1_bus = {
    .i2c      = I2C1,
    .callback = rhs_hal_i2c_bus_external_event,
};

void rhs_hal_i2c_bus_handle_event(const RHSHalI2cBusHandle* handle, RHSHalI2cBusHandleEvent event)
{
    if (event == RHSHalI2cBusHandleEventActivate)
    {
#if defined(STM32F103xE)
        // Enable GPIOB clock
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

        // Configure PB6 (SCL) and PB7 (SDA) as alternate function open drain
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_OPENDRAIN);
        LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_LOW);

        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_OPENDRAIN);
        LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_LOW);
#elif defined(STM32F765xx)
        // Enable GPIOB clock
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

        // Configure PB8 (SCL) and PB9 (SDA) as alternate function open drain
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_8, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_8, LL_GPIO_AF_4);
        LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_8, LL_GPIO_OUTPUT_OPENDRAIN);
        LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_8, LL_GPIO_PULL_NO);
        LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_8, LL_GPIO_SPEED_FREQ_LOW);

        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
        LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_4);
        LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
        LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_NO);
        LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_LOW);
#endif

#if defined(STM32F103xE)
        LL_I2C_InitTypeDef I2C_InitStruct;
        I2C_InitStruct.PeripheralMode  = LL_I2C_MODE_I2C;
        I2C_InitStruct.ClockSpeed      = 100000;  // 100kHz
        I2C_InitStruct.DutyCycle       = LL_I2C_DUTYCYCLE_2;
        I2C_InitStruct.OwnAddress1     = 0;
        I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
        I2C_InitStruct.OwnAddrSize     = LL_I2C_OWNADDRESS1_7BIT;
        LL_I2C_Init(handle->bus->i2c, &I2C_InitStruct);
        LL_I2C_Enable(handle->bus->i2c);
#elif defined(STM32F765xx)
        LL_I2C_InitTypeDef I2C_InitStruct;
        I2C_InitStruct.PeripheralMode  = LL_I2C_MODE_I2C;
        I2C_InitStruct.AnalogFilter    = LL_I2C_ANALOGFILTER_ENABLE;
        I2C_InitStruct.DigitalFilter   = 0;
        I2C_InitStruct.OwnAddress1     = 0;
        I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
        I2C_InitStruct.OwnAddrSize     = LL_I2C_OWNADDRESS1_7BIT;
        I2C_InitStruct.Timing          = RHS_HAL_I2C_CONFIG_POWER_I2C_TIMINGS_100;
        LL_I2C_Init(handle->bus->i2c, &I2C_InitStruct);
        // I2C is enabled at this point
        LL_I2C_EnableAutoEndMode(handle->bus->i2c);
        LL_I2C_SetOwnAddress2(handle->bus->i2c, 0, LL_I2C_OWNADDRESS2_NOMASK);
        LL_I2C_DisableOwnAddress2(handle->bus->i2c);
        LL_I2C_DisableGeneralCall(handle->bus->i2c);
        LL_I2C_EnableClockStretching(handle->bus->i2c);
#endif
    }
    else if (event == RHSHalI2cBusHandleEventDeactivate)
    {
        LL_I2C_GenerateStopCondition(handle->bus->i2c);
        LL_I2C_Disable(handle->bus->i2c);
        
        // GPIO deinitialization for I2C1: PB8 (SCL), PB9 (SDA) - set to analog mode
#if defined(STM32F103xE)
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ANALOG);
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ANALOG);
#elif defined(STM32F765xx)
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_8, LL_GPIO_MODE_ANALOG);
        LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ANALOG);
#endif
    }
}

const RHSHalI2cBusHandle rhs_hal_i2c1_handle = {
    .bus      = &rhs_hal_i2c1_bus,
    .callback = rhs_hal_i2c_bus_handle_event,
};
