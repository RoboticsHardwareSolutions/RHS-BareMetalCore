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
    /**
     * @brief Wait for any flag condition (default behavior)
     *
     * When this flag is set, the wait operation will return as soon as
     * any of the specified flags becomes available. This is the default
     * waiting mode when no specific wait type is specified.
     */
    RHSFlagWaitAny = 0x00000000U,

    /**
     * @brief Wait for all flags condition
     *
     * When this flag is set, the wait operation will only return when
     * ALL of the specified flags become available simultaneously.
     * This ensures that all required conditions are met before proceeding.
     */
    RHSFlagWaitAll = 0x00000001U,

    /**
     * @brief Do not clear flags after wait operation
     *
     * By default, flags are automatically cleared after a successful wait.
     * Setting this flag prevents the automatic clearing, allowing the flags
     * to remain set for subsequent operations or other waiting tasks.
     */
    RHSFlagNoClear = 0x00000002U,

    /**
     * @brief General error indicator flag
     *
     * This flag indicates that an error condition has occurred during
     * the flag operation. The specific error type can be determined
     * by checking additional error flags or status codes.
     */
    RHSFlagError = 0x80000000U,

    /**
     * @brief Unknown or unspecified error occurred
     *
     * Represents a general error condition where the specific cause
     * cannot be determined or doesn't fit into other error categories.
     * Corresponds to RHSStatusError (-1).
     */
    RHSFlagErrorUnknown = 0xFFFFFFFFU,

    /**
     * @brief Operation timed out
     *
     * The flag wait operation exceeded the specified timeout period
     * without the required flags becoming available. This prevents
     * indefinite blocking when flags are not set within expected time.
     * Corresponds to RHSStatusErrorTimeout (-2).
     */
    RHSFlagErrorTimeout = 0xFFFFFFFEU,

    /**
     * @brief Required resource is not available
     *
     * The operation failed because a necessary system resource
     * (memory, handles, etc.) is not available or has been exhausted.
     * Corresponds to RHSStatusErrorResource (-3).
     */
    RHSFlagErrorResource = 0xFFFFFFFDU,

    /**
     * @brief Invalid parameter provided
     *
     * One or more parameters passed to the flag operation are invalid,
     * out of range, or incompatible with the current system state.
     * Corresponds to RHSStatusErrorParameter (-4).
     */
    RHSFlagErrorParameter = 0xFFFFFFFCU,

    /**
     * @brief Operation not allowed in ISR context
     *
     * The flag operation was attempted from within an Interrupt Service
     * Routine (ISR) context where such operations are not permitted.
     * Use ISR-safe alternatives when calling from interrupt handlers.
     * Corresponds to RHSStatusErrorISR (-6).
     */
    RHSFlagErrorISR = 0xFFFFFFFAU,
} RHSFlag;

typedef enum
{
    /**
     * @brief Operation completed successfully
     *
     * The requested operation was executed without any errors
     * and completed as expected. All outputs are valid and
     * the system state has been updated accordingly.
     */
    RHSStatusOk = 0x00000000U,

    /**
     * @brief Unspecified RTOS error occurred
     *
     * A run-time error occurred during the operation, but the specific
     * error type doesn't match any of the other defined error categories.
     * This represents a general failure condition that requires investigation.
     */
    RHSStatusError = 0xFFFFFFFFU,

    /**
     * @brief Operation timed out
     *
     * The requested operation could not be completed within the
     * specified timeout period. This prevents indefinite blocking
     * and allows the application to handle delayed operations gracefully.
     */
    RHSStatusErrorTimeout = 0xFFFFFFFEU,

    /**
     * @brief Required resource is unavailable
     *
     * The operation failed because a necessary system resource
     * (such as memory, semaphores, or hardware peripherals) is
     * currently unavailable or has been exhausted.
     */
    RHSStatusErrorResource = 0xFFFFFFFDU,

    /**
     * @brief Invalid parameter error
     *
     * One or more parameters passed to the function are invalid,
     * out of range, NULL when a valid pointer is required, or
     * incompatible with the current system configuration.
     */
    RHSStatusErrorParameter = 0xFFFFFFFCU,

    /**
     * @brief System out of memory
     *
     * The system has insufficient memory available to complete
     * the requested operation. This could be due to memory
     * fragmentation or actual memory exhaustion. Memory allocation
     * or reservation for the operation was impossible.
     */
    RHSStatusErrorNoMemory = 0xFFFFFFFBU,

    /**
     * @brief Operation not allowed in ISR context
     *
     * The function was called from within an Interrupt Service Routine
     * (ISR) context where such operations are prohibited. Use ISR-safe
     * function variants or defer the operation to a task context.
     */
    RHSStatusErrorISR = 0xFFFFFFFAU,

    /**
     * @brief Reserved value to prevent enum optimization
     *
     * This value ensures that the compiler treats this enum as
     * a 32-bit type and prevents potential size optimization that
     * could cause compatibility issues with the RTOS interface.
     */
    RHSStatusReserved = 0x7FFFFFFF
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