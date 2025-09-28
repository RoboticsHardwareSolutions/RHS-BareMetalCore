#pragma once

#if defined(STM32F103xE)
#    include <stm32f1xx_ll_i2c.h>
#elif defined(STM32F407xx)
#    include <stm32f4xx_ll_i2c.h>
#elif defined(STM32F765xx)
#    include <stm32f7xx_ll_i2c.h>
#else
#    error "Unsupported platform"
#endif


typedef struct RHSHalI2cBus       RHSHalI2cBus;
typedef struct RHSHalI2cBusHandle RHSHalI2cBusHandle;

/** RHSHal i2c bus states */
typedef enum
{
    RHSHalI2cBusEventInit,       /**< Bus initialization event, called on system start */
    RHSHalI2cBusEventDeinit,     /**< Bus deinitialization event, called on system stop */
    RHSHalI2cBusEventLock,       /**< Bus lock event, called before activation */
    RHSHalI2cBusEventUnlock,     /**< Bus unlock event, called after deactivation */
    RHSHalI2cBusEventActivate,   /**< Bus activation event, called before handle activation */
    RHSHalI2cBusEventDeactivate, /**< Bus deactivation event, called after handle deactivation  */
} RHSHalI2cBusEvent;

/** RHSHal i2c bus event callback */
typedef void (*RHSHalI2cBusEventCallback)(RHSHalI2cBus* bus, RHSHalI2cBusEvent event);

/** RHSHal i2c bus */
struct RHSHalI2cBus
{
    I2C_TypeDef*              i2c;
    const RHSHalI2cBusHandle* current_handle;
    RHSHalI2cBusEventCallback callback;
};

/** RHSHal i2c handle states */
typedef enum
{
    RHSHalI2cBusHandleEventActivate,   /**< Handle activate: connect gpio and apply bus config */
    RHSHalI2cBusHandleEventDeactivate, /**< Handle deactivate: disconnect gpio and reset bus config */
} RHSHalI2cBusHandleEvent;

/** RHSHal i2c handle event callback */
typedef void (*RHSHalI2cBusHandleEventCallback)(const RHSHalI2cBusHandle* handle, RHSHalI2cBusHandleEvent event);

/** RHSHal i2c handle */
struct RHSHalI2cBusHandle
{
    RHSHalI2cBus*                   bus;
    RHSHalI2cBusHandleEventCallback callback;
};

#ifdef __cplusplus
}
#endif
