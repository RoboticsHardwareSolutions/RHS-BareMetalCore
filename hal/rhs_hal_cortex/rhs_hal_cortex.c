#include "rhs_hal_cortex.h"
#include "rhs.h"
#ifdef STM32F765xx
#    include "stm32f7xx.h"
#elif STM32F407xx
#    include "stm32f4xx.h"
#elif STM32F103xE
#    include "stm32f103xe.h"
#else
#endif

#define RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND (SystemCoreClock / 1000000)

void rhs_hal_cortex_init_early(void)
{
#ifdef STM32F765xx
    SCB_EnableICache();
    SCB_EnableDCache();
#endif
    CoreDebug->DEMCR |= (CoreDebug_DEMCR_TRCENA_Msk | CoreDebug_DEMCR_MON_EN_Msk);
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
}

void rhs_hal_cortex_delay_us(uint32_t microseconds)
{
    rhs_assert(microseconds < (UINT32_MAX / RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND));

    uint32_t start      = DWT->CYCCNT;
    uint32_t time_ticks = RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND * microseconds;

    while ((DWT->CYCCNT - start) < time_ticks)
    {
    }
}

__attribute__((warn_unused_result)) RHSHalCortexTimer rhs_hal_cortex_timer_get(uint32_t timeout_us)
{
    rhs_assert(timeout_us < (UINT32_MAX / RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND));

    RHSHalCortexTimer cortex_timer = {0};
    cortex_timer.start             = DWT->CYCCNT;
    cortex_timer.value             = RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND * timeout_us;
    return cortex_timer;
}

bool rhs_hal_cortex_timer_is_expired(RHSHalCortexTimer cortex_timer)
{
    return !((DWT->CYCCNT - cortex_timer.start) < cortex_timer.value);
}

void rhs_hal_cortex_timer_wait(RHSHalCortexTimer cortex_timer)
{
    while (!rhs_hal_cortex_timer_is_expired(cortex_timer))
        ;
}
