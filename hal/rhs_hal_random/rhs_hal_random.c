#include <rhs.h>
#include <rhs_hal_random.h>

#if defined(STM32F765xx)
#    include "stm32f7xx_ll_bus.h"
#    include "stm32f7xx_ll_rcc.h"
#    include "stm32f7xx_ll_rng.h"
#elif defined(STM32F407xx) || defined(STM32F405xx)
#    include "stm32f4xx.h"
#    include "stm32f4xx_ll_bus.h"
#    include "stm32f4xx_ll_rcc.h"
#    include "stm32f4xx_ll_rng.h"
#elif defined(STM32F103xE)
#    include "stm32f1xx.h"
#    include "stm32f1xx_ll_bus.h"
#    include "stm32f1xx_ll_rcc.h"
#else
#    error "No processor defined or not implemented"
#endif

#define TAG "RHSHalRandom"

#if defined(STM32F765xx) || defined(STM32F407xx)
static uint32_t rhs_hal_random_read_rng(void)
{
    while (LL_RNG_IsActiveFlag_CECS(RNG) || LL_RNG_IsActiveFlag_SECS(RNG) || !LL_RNG_IsActiveFlag_DRDY(RNG))
    {
        /* Error handling as described in RM0434, pg. 582-583 */
        if (LL_RNG_IsActiveFlag_CECS(RNG))
        {
            /* Clock error occurred */
            LL_RNG_ClearFlag_CEIS(RNG);
        }

        if (LL_RNG_IsActiveFlag_SECS(RNG))
        {
            /* Noise source error occurred */
            LL_RNG_ClearFlag_SEIS(RNG);

            for (uint32_t i = 0; i < 12; ++i)
            {
                const volatile uint32_t discard = LL_RNG_ReadRandData32(RNG);
                UNUSED(discard);
            }
        }
    }

    return LL_RNG_ReadRandData32(RNG);
}

void rhs_hal_random_init(void)
{
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_RNG);
#    if defined(STM32F765xx)
    LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_PLL);
#    endif
}

uint32_t rhs_hal_random_get(void)
{
    // while (LL_HSEM_1StepLock(HSEM, CFG_HW_RNG_SEMID))
    // ;
    LL_RNG_Enable(RNG);

    const uint32_t random_val = rhs_hal_random_read_rng();

    LL_RNG_Disable(RNG);
    // ;
    // LL_HSEM_ReleaseLock(HSEM, CFG_HW_RNG_SEMID, 0);

    return random_val;
}

void rhs_hal_random_fill_buf(uint8_t* buf, uint32_t len)
{
    rhs_assert(buf);
    rhs_assert(len);

    // while (LL_HSEM_1StepLock(HSEM, CFG_HW_RNG_SEMID))
    // ;
    LL_RNG_Enable(RNG);

    for (uint32_t i = 0; i < len; i += 4)
    {
        const uint32_t random_val = rhs_hal_random_read_rng();
        uint8_t        len_cur    = ((i + 4) < len) ? (4) : (len - i);
        memcpy(&buf[i], &random_val, len_cur);
    }

    LL_RNG_Disable(RNG);
    // LL_HSEM_ReleaseLock(HSEM, CFG_HW_RNG_SEMID, 0);
}

int __wrap_rand(void)
{
    return rhs_hal_random_get() & RAND_MAX;
}

#elif defined(STM32F103xE)

static uint32_t rng_state = 1;

static uint32_t rhs_hal_random_read_rng(void)
{
    rng_state = rng_state * 1103515245 + 12345;
    return rng_state;
}

void rhs_hal_random_init(void)
{
    rng_state = SysTick->VAL;
}

uint32_t rhs_hal_random_get(void)
{
    return rhs_hal_random_read_rng();
}

void rhs_hal_random_fill_buf(uint8_t* buf, uint32_t len)
{
    rhs_assert(buf);
    rhs_assert(len);

    for (uint32_t i = 0; i < len; i += 4)
    {
        const uint32_t random_val = rhs_hal_random_read_rng();
        uint8_t        len_cur    = ((i + 4) < len) ? (4) : (len - i);
        memcpy(&buf[i], &random_val, len_cur);
    }
}

int __wrap_rand(void)
{
    return rhs_hal_random_get() & RAND_MAX;
}
#endif