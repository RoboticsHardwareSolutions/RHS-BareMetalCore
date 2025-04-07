#pragma once
#include "stdint.h"
#include "stdbool.h"

/** Cortex timer provides high precision low level expiring timer */
typedef struct
{
    uint32_t start;
    uint32_t value;
} RHSHalCortexTimer;

void rhs_hal_cortex_init_early(void);

void rhs_hal_cortex_delay_us(uint32_t microseconds);

RHSHalCortexTimer rhs_hal_cortex_timer_get(uint32_t timeout_us);

bool rhs_hal_cortex_timer_is_expired(RHSHalCortexTimer cortex_timer);

void rhs_hal_cortex_timer_wait(RHSHalCortexTimer cortex_timer);
