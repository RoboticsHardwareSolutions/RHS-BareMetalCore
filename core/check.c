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

#ifdef STM32F765xx
#    include "stm32f7xx.h"
#elif STM32F407xx || defined(STM32F405xx)
#    include "stm32f4xx.h"
#elif STM32F103xE
#    include "stm32f1xx.h"
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

#define STORE_STACK_PTR(_x)               \
    __asm volatile("TST LR, #4 \n"        \
                   "ITE EQ \n"            \
                   "MRSEQ %[ptr], MSP \n" \
                   "MRSNE %[ptr], PSP \n" \
                   : [ptr] "=r"(_x))

static void rhs_save_stack_info(void)
{
    stack_ptr_t* stack_ptr;
    STORE_STACK_PTR(stack_ptr);

    if (rhs_log_save)
    {
        rhs_log_save("r0=%08lX r1=%08lX r2=%08lX r3=%08lX r12=%08lX lr=%08lX pc=%08lX psr=%08lX",
                     (unsigned long) stack_ptr->r0,
                     (unsigned long) stack_ptr->r1,
                     (unsigned long) stack_ptr->r2,
                     (unsigned long) stack_ptr->r3,
                     (unsigned long) stack_ptr->r12,
                     (unsigned long) stack_ptr->lr,
                     (unsigned long) stack_ptr->pc,
                     (unsigned long) stack_ptr->psr);
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
    bool debug = CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk;

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
    __builtin_unreachable();
}
