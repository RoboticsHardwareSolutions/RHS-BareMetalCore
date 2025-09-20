#pragma once
#include "rhs_hal_interrupt.h"
#include "rhs_hal_cortex.h"
#include "rhs_hal_version.h"
#include "rhs_hal_power.h"

#if RHS_HAL_SPEAKER
#    include "rhs_hal_speaker.h"
#endif
#if RHS_HAL_FLASH_EX
#    include "rhs_hal_flash_ex.h"
#endif
#if RHS_HAL_RTC
#    include "rhs_hal_rtc.h"
#endif
#if RHS_HAL_IO
#    include "rhs_hal_io.h"
#endif
#if RHS_HAL_SERIAL
#    include "rhs_hal_serial.h"
#endif
#if RHS_HAL_CAN
#    include "rhs_hal_can.h"
#endif
#if RHS_HAL_RANDOM
#    include "rhs_hal_random.h"
#endif
#if RHS_HAL_USB
#    include "rhs_hal_usb.h"
#    include "rhs_hal_usb_cdc.h"
#endif

void rhs_hal_init(void);
