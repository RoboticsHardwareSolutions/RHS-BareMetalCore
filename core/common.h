#pragma once
#include <cmsis_compiler.h>
#include "stdbool.h"

#ifndef RHS_IS_IRQ_MODE
#define RHS_IS_IRQ_MODE() (__get_IPSR() != 0U)
#endif

#ifndef RHS_IS_IRQ_MASKED
#define RHS_IS_IRQ_MASKED() (__get_PRIMASK() != 0U)
#endif

#ifndef RHS_IS_ISR
#define RHS_IS_ISR() (RHS_IS_IRQ_MODE() || RHS_IS_IRQ_MASKED())
#endif


typedef struct {
    uint32_t isrm;
    bool from_isr;
    bool kernel_running;
} __RHSCriticalInfo;

__RHSCriticalInfo __rhs_critical_enter(void);

void __rhs_critical_exit(__RHSCriticalInfo info);

#ifndef RHS_CRITICAL_ENTER
#define RHS_CRITICAL_ENTER() __RHSCriticalInfo __rhs_critical_info = __rhs_critical_enter();
#endif

#ifndef RHS_CRITICAL_EXIT
#define RHS_CRITICAL_EXIT() __rhs_critical_exit(__rhs_critical_info);
#endif
