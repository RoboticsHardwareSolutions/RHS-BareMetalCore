#pragma once
#include "stdint.h"

void rhs_hal_random_init(void);

uint32_t rhs_hal_random_get(void);

void rhs_hal_random_fill_buf(uint8_t* buf, uint32_t len);

int rand(void);
