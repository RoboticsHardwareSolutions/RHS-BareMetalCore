#pragma once
#include "unixtime.h"
#include "rhs.h"

void rhs_hal_rtc_init(void);

void rhs_hal_rtc_set_datetime(datetime_t* datetime);

void rhs_hal_rtc_get_datetime(datetime_t* out_datetime);

void rhs_hal_rtc_set_timestamp(uint64_t seconds, uint32_t mseconds);

void rhs_hal_rtc_get_timestamp(uint64_t* out_seconds, uint32_t* out_mseconds);
