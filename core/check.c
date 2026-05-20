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

_Noreturn void __rhs_crash_implementation(const char* file, int line, char* m)
{
    __disable_irq();

    static volatile bool in_crash = false;
    if (in_crash)
    {
        rhs_hal_power_reset();
        __builtin_unreachable();
    }
    in_crash = true;

    if (rhs_crash_action)
        rhs_crash_action();

    bool isr = RHS_IS_IRQ_MODE();

    if (!isr)
    {
    }

    const char* last_separator = strrchr(file, '/');
    if (last_separator != NULL)
    {
        file = last_separator + 1;
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
                         file,
                         line);
        }
    }
#else
    if (rhs_log_save)
    {
        rhs_log_save("%u: msg: %s. file: %s, line: %d;", rhs_get_tick(), m, file, line);
    }
#endif
    rhs_save_stack_info();
    RHS_LOG_D("Assert", "Message: %s. Called from file: %s, line: %d\n", m, file, line);

// Halt CPU (breakpoint) when hitting error, only apply for Cortex M3, M4, M7, M33. M55
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M_MAIN__) ||                      \
    defined(__ARM_ARCH_8_1M_MAIN__) || defined(__ARM7M__) || defined(__ARM7EM__) || defined(__ARM8M_MAINLINE__) || \
    defined(__ARM8EM_MAINLINE__)
    bool debug = false;
#    if defined(CoreDebug) && defined(CoreDebug_DHCSR_C_DEBUGEN_Msk)
    debug = (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0U;
#    else
    volatile uint32_t* ARM_CM_DHCSR = ((volatile uint32_t*) 0xE000EDF0UL); /* Cortex M CoreDebug->DHCSR */
    debug                           = (0u != ((*ARM_CM_DHCSR) & 1UL));
#    endif
    if (debug)
        __asm("BKPT #0\n");

#elif defined(__riscv) && !defined(ESP_PLATFORM)
    __asm("ebreak\n");

#elif defined(_mips)
    __asm("sdbbp 0");

#else // For other architectures infinite loop is used to halt the CPU, 
    for (;;)
    {
    }
#endif
    rhs_hal_power_reset();
    
    __builtin_unreachable();
}


