#pragma once
#include "unixtime.h"
#include "rhs.h"

typedef enum
{
    RHSHalRtcRegisterSystem,

    RHSHalRtcRegisterMAX,
} RHSHalRtcRegister;

void rhs_hal_rtc_init(void);

void rhs_hal_rtc_set_datetime(datetime_t* datetime);

void rhs_hal_rtc_get_datetime(datetime_t* out_datetime);

void rhs_hal_rtc_set_timestamp(uint64_t seconds, uint32_t mseconds);

void rhs_hal_rtc_get_timestamp(uint64_t* out_seconds, uint32_t* out_mseconds);

/** Get RTC register content
 *
 * @param[in]  reg   The register identifier
 *
 * @return     content of the register
 */
uint32_t rhs_hal_rtc_get_register(RHSHalRtcRegister reg);

/** Set register content
 *
 * @param[in]  reg    The register identifier
 * @param[in]  value  The value to store into register
 */
void rhs_hal_rtc_set_register(RHSHalRtcRegister reg, uint32_t value);
