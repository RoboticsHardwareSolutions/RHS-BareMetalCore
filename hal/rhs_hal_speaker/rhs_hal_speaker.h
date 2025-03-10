#pragma once
#include "stdint.h"
#include <stdbool.h>

void rhs_hal_speaker_init(void);

bool rhs_hal_speaker_acquire(uint32_t timeout);

void rhs_hal_speaker_release(void);

bool rhs_hal_speaker_is_mine(void);

void rhs_hal_speaker_start(float frequency, float volume);

void rhs_hal_speaker_stop(void);
