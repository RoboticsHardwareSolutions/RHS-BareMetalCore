#include "rhs_hal_power.h"

#if defined(RPLC_XL) || defined(RPLC_L)
#    include "stm32f765xx.h"
#elif defined(RPLC_M)
#    include "stm32f103xe.h"
#else
#    if STM32F407xx
#        include "stm32f4xx.h"
#    endif
#endif

_Noreturn void rhs_hal_power_reset(void)
{
#ifdef STM32F765xx
    SCB_CleanInvalidateDCache();
#endif
    NVIC_SystemReset();
}
