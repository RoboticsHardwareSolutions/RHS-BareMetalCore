#include "rhs_hal.h"

void rhs_hal_init(void)
{
    rhs_hal_cortex_init_early();
    rhs_hal_interrupt_init();
#ifdef RPLC_XL
    rhs_hal_io_init();
    rhs_hal_flash_ex_init();
    rhs_hal_speaker_init();
    rhs_hal_random_init();
    rhs_hal_rtc_init();
#elif RPLC_L
    rhs_hal_io_init();
    rhs_hal_flash_ex_init();
    rhs_hal_speaker_init();
    rhs_hal_random_init();
    rhs_hal_rtc_init();
#elif RPLC_M
    rhs_hal_speaker_init();
#else
#    if RHS_HAL_SPEAKER
    rhs_hal_speaker_init();
#    endif
#    if RHS_HAL_FLASH_EX
    rhs_hal_flash_ex_init();
#    endif
#    if RHS_HAL_RTC
#    endif
#    if RHS_HAL_IO
    rhs_hal_io_init();
#    endif
#    if RHS_HAL_SERIAL
#    endif
#    if RHS_HAL_CAN
#    endif
#    if RHS_HAL_NETWORK
    rhs_hal_network_init();
#    endif
#    if RHS_HAL_RANDOM
    rhs_hal_random_init();
#    endif
#endif
}
