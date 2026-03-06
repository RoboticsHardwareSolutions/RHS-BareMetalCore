#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "check.h"
#include "stdint.h"
#include "common.h"
#include "kernel.h"
#include "log.h"
#include "rhs_hal_power.h"
#if defined(TIMESTAMPER_RTC)
#    include "rhs_hal_rtc.h"
#endif

#if defined(STM32F765xx)
#    include "stm32f7xx.h"
#elif defined(STM32F407xx) || defined(STM32F405xx)
#    include "stm32f4xx.h"
#elif defined(STM32F103xE)
#    include "stm32f1xx.h"
#elif defined(STM32G0B1xx)
#    include "stm32g0xx.h"
#endif

typedef struct __attribute__((packed))
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} stack_ptr_t;

/* Exception frame pointer captured by a naked fault entry before any C stack
 * activity.  NULL when the crash originates from a software assert. */
static uint32_t* rhs_fault_frame = NULL;

void rhs_set_fault_frame(uint32_t* frame)
{
    rhs_fault_frame = frame;
}

static void rhs_save_stack_info(void)
{
    if (rhs_fault_frame != NULL)
    {
        const stack_ptr_t* f = (const stack_ptr_t*) rhs_fault_frame;
        printf("r0=%08lX r1=%08lX r2=%08lX r3=%08lX r12=%08lX lr=%08lX pc=%08lX psr=%08lX\n",
               (unsigned long) f->r0,
               (unsigned long) f->r1,
               (unsigned long) f->r2,
               (unsigned long) f->r3,
               (unsigned long) f->r12,
               (unsigned long) f->lr,
               (unsigned long) f->pc,
               (unsigned long) f->psr);

        if (rhs_log_save)
            rhs_log_save("r0=%08lX r1=%08lX r2=%08lX r3=%08lX r12=%08lX lr=%08lX pc=%08lX psr=%08lX",
                         (unsigned long) f->r0,
                         (unsigned long) f->r1,
                         (unsigned long) f->r2,
                         (unsigned long) f->r3,
                         (unsigned long) f->r12,
                         (unsigned long) f->lr,
                         (unsigned long) f->pc,
                         (unsigned long) f->psr);
    }
}

_Noreturn void __rhs_crash_implementation(CallContext context, char* m)
{
    __disable_irq();

    if (rhs_crash_action)
        rhs_crash_action();

    bool isr = RHS_IS_IRQ_MODE();

    if (!isr)
    {
    }

    // Check if debug enabled by DAP
    // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/Debug-register-support-in-the-SCS/Debug-Halting-Control-and-Status-Register--DHCSR?lang=en
#if defined(CoreDebug) && defined(CoreDebug_DHCSR_C_DEBUGEN_Msk)
    bool debug = (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U;
#endif

    const char* last_separator = strrchr(context.file, '/');
    if (last_separator != NULL)
    {
        context.file = last_separator + 1;
    }
#if defined(TIMESTAMPER_RTC)
    {
        datetime_t datetime;
        rhs_hal_rtc_get_datetime(&datetime);
        if (rhs_log_save)
        {
            rhs_log_save("%04u-%02u-%02u %02u:%02u:%02u: msg: %s. file: %s, line: %d;",
                         (uint32_t) datetime.year,
                         (uint32_t) datetime.month,
                         (uint32_t) datetime.day,
                         (uint32_t) datetime.hours,
                         (uint32_t) datetime.minutes,
                         (uint32_t) datetime.seconds,
                         m,
                         context.file,
                         context.line);
        }
    }
#else
    if (rhs_log_save)
    {
        rhs_log_save("%u: msg: %s. file: %s, line: %d;", rhs_get_tick(), m, context.file, context.line);
    }
#endif
    rhs_save_stack_info();
    RHS_LOG_D("Assert", "Message: %s. Called from file: %s, line: %d\n", m, context.file, context.line);

    #if defined(CoreDebug) && defined(CoreDebug_DHCSR_C_DEBUGEN_Msk)
    if (debug)
    {
        for (;;)
        {
        }
    }
    else
    {
        rhs_hal_power_reset();
    }
#else
    // __BKPT(0U);
    rhs_hal_power_reset();
#endif
    __builtin_unreachable();
}
