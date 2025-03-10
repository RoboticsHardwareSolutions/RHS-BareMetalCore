#pragma once
#include "assert.h"

typedef struct
{
    const char* file;
    int         line;
} CallContext;

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
