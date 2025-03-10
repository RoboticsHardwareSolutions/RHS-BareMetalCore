#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    RHSWaitForever = 0xFFFFFFFFU,
} RHSWait;

typedef enum
{
    RHSFlagWaitAny = 0x00000000U,  ///< Wait for any flag (default).
    RHSFlagWaitAll = 0x00000001U,  ///< Wait for all flags.
    RHSFlagNoClear = 0x00000002U,  ///< Do not clear flags which have been specified to wait for.

    RHSFlagError          = 0x80000000U,  ///< Error indicator.
    RHSFlagErrorUnknown   = 0xFFFFFFFFU,  ///< RHSStatusError (-1).
    RHSFlagErrorTimeout   = 0xFFFFFFFEU,  ///< RHSStatusErrorTimeout (-2).
    RHSFlagErrorResource  = 0xFFFFFFFDU,  ///< RHSStatusErrorResource (-3).
    RHSFlagErrorParameter = 0xFFFFFFFCU,  ///< RHSStatusErrorParameter (-4).
    RHSFlagErrorISR       = 0xFFFFFFFAU,  ///< RHSStatusErrorISR (-6).
} RHSFlag;

typedef enum
{
    RHSStatusOk             = 0,   ///< Operation completed successfully.
    RHSStatusError          = -1,  ///< Unspecified RTOS error: run-time error but no other error message fits.
    RHSStatusErrorTimeout   = -2,  ///< Operation not completed within the timeout period.
    RHSStatusErrorResource  = -3,  ///< Resource not available.
    RHSStatusErrorParameter = -4,  ///< Parameter error.
    RHSStatusErrorNoMemory =
        -5,  ///< System is out of memory: it was impossible to allocate or reserve memory for the operation.
    RHSStatusErrorISR =
        -6,  ///< Not allowed in ISR context: the function cannot be called from interrupt service routines.
    RHSStatusReserved = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
} RHSStatus;

typedef enum
{
    RHSSignalExit, /**< Request (graceful) exit. */
    // Other standard signals may be added in the future
    RHSSignalCustom = 100, /**< Custom signal values start from here. */
} RHSSignal;

#ifdef __cplusplus
}
#endif
