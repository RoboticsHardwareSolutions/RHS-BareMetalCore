#include "rhs_hal_power.h"

#if defined(BMPLC_XL) || defined(BMPLC_L)
#    include "stm32f765xx.h"
#elif defined(BMPLC_M)
#    include "stm32f103xe.h"
#else
#    if defined(STM32F407xx)  || defined(STM32F405xx)
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
