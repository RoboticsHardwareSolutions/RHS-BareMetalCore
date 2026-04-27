#include "rhs_hal_cortex.h"
#include "rhs.h"

#if defined(STM32F765xx)
#    include "stm32f7xx.h"
#    include "stm32f7xx_hal_cortex.h"
#elif defined(STM32F407xx)
#    include "stm32f4xx.h"
#elif defined(STM32F103xE)
#    include "stm32f103xe.h"
#elif defined(STM32G0B1xx)
#    include "stm32g0b1xx.h"
#else
#endif

#define RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND (SystemCoreClock / 1000000)

/* Linker script symbols for SRAM2 region */
extern uint32_t _sram2_start;
extern uint32_t _sram2_end;
extern uint32_t _sram2_size;

#ifdef STM32F765xx
static void rhs_hal_cortex_configure_mpu(void)
{
    MPU_Region_InitTypeDef mpu = {0};

    HAL_MPU_Disable();

    /*
     * Mark SRAM2 (0x2007C000..0x20080000, 16KB) as non-cacheable.
     * MPU requires power-of-two region size and alignment.
     * Base address and size are taken from linker script symbols.
     */
    mpu.Enable           = MPU_REGION_ENABLE;
    mpu.Number           = MPU_REGION_NUMBER7;
    mpu.BaseAddress      = (uint32_t)&_sram2_start;
    mpu.Size             = MPU_REGION_SIZE_16KB;
    mpu.SubRegionDisable = 0x00;
    mpu.TypeExtField     = MPU_TEX_LEVEL1;
    mpu.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
    mpu.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;
    mpu.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    mpu.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&mpu);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
#endif

void rhs_hal_cortex_init_early(void)
{
#ifdef STM32F765xx
    rhs_hal_cortex_configure_mpu();
    SCB_EnableICache();
    SCB_EnableDCache();
#endif

#if !defined(STM32G0B1xx)
    CoreDebug->DEMCR |= (CoreDebug_DEMCR_TRCENA_Msk | CoreDebug_DEMCR_MON_EN_Msk);
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
#else
    /* Cortex-M0+ has no DWT CYCCNT; use TIM2 as a free-running 1 MHz counter */
    RCC->APBENR1 |= RCC_APBENR1_TIM2EN;
    TIM2->PSC = (SystemCoreClock / 1000000U) - 1U;
    TIM2->ARR = 0xFFFFFFFFU;
    TIM2->EGR = TIM_EGR_UG; /* force prescaler reload */
    TIM2->CR1 = TIM_CR1_CEN;
#endif
}

void rhs_hal_cortex_delay_us(uint32_t microseconds)
{
    rhs_assert(microseconds < (UINT32_MAX / RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND));
#if !defined(STM32G0B1xx)
    uint32_t start      = DWT->CYCCNT;
    uint32_t time_ticks = RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND * microseconds;

    while ((DWT->CYCCNT - start) < time_ticks)
    {
    }
#else
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < microseconds)
    {
    }
#endif
}

__attribute__((warn_unused_result)) RHSHalCortexTimer rhs_hal_cortex_timer_get(uint32_t timeout_us)
{
    rhs_assert(timeout_us < (UINT32_MAX / RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND));

    RHSHalCortexTimer cortex_timer = {0};
#if !defined(STM32G0B1xx)
    cortex_timer.start = DWT->CYCCNT;
    cortex_timer.value = RHS_HAL_CORTEX_INSTRUCTIONS_PER_MICROSECOND * timeout_us;
#else
    cortex_timer.start = TIM2->CNT;
    cortex_timer.value = timeout_us;
#endif
    return cortex_timer;
}

bool rhs_hal_cortex_timer_is_expired(RHSHalCortexTimer cortex_timer)
{
#if !defined(STM32G0B1xx)
    return !((DWT->CYCCNT - cortex_timer.start) < cortex_timer.value);
#else
    return (TIM2->CNT - cortex_timer.start) >= cortex_timer.value;
#endif
}

void rhs_hal_cortex_timer_wait(RHSHalCortexTimer cortex_timer)
{
    while (!rhs_hal_cortex_timer_is_expired(cortex_timer))
        ;
}
