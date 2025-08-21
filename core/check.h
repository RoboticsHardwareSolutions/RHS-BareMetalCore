#pragma once
#include "assert.h"

typedef struct
{
    const char* file;
    int         line;
} CallContext;

/**
 * @brief Custom logging function for critical errors and assertions
 * 
 * This function is marked as weak, allowing users to provide their own implementation.
 * If you want to define custom logging behavior for critical errors and assertions,
 * implement this function in your code:
 * 
 * @code
 * void rhs_log_save(char* str, ...) {
 *     // Your custom logging implementation here
 *     // For example: save to flash, send via UART, etc.
 * }
 * @endcode
 * 
 * @param str Format string (printf-style)
 * @param ... Variable arguments for format string
 * 
 * @note If not implemented by user, this function will have no effect
 */
__attribute__((weak)) void rhs_log_save(char* str, ...);

_Noreturn void __rhs_crash_implementation(CallContext context, char* m);

#define rhs_assert(x)                                  \
    do                                                 \
    {                                                  \
        CallContext ctx = {__FILE__, __LINE__};        \
        if ((x) == 0)                                  \
        {                                              \
            __rhs_crash_implementation(ctx, "Assert"); \
        }                                              \
    } while (0)

/** Crash system with message. Show message after reboot. */
#define rhs_crash(m)                            \
    do                                          \
    {                                           \
        CallContext ctx = {__FILE__, __LINE__}; \
        __rhs_crash_implementation(ctx, (m));   \
    } while (0)
