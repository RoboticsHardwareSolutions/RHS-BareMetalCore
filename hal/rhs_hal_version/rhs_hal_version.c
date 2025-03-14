#include "rhs_hal_version.h"
#if defined(STM32F765xx)
#include "stm32f765xx.h"
#elif defined(STM32F407xx)
#include "stm32f407xx.h"
#elif defined(STM32F103xE)
#include "stm32f103xe.h"
#endif

const uint8_t* rhs_hal_version_uid(void)
{
    return (const uint8_t*) UID_BASE;
}
