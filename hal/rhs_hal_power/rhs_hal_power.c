#include "rhs_hal_power.h"

#ifdef STM32F765xx
#    include "stm32f765xx.h"
#elif STM32F407xx
#    include "stm32f4xx.h"
#endif

_Noreturn void rhs_hal_power_reset(void)
{
#ifdef STM32F765xx
    SCB_CleanInvalidateDCache();
#endif
    NVIC_SystemReset();
}
