#include "common.h"

#include <FreeRTOS.h>
#include <task.h>

__RHSCriticalInfo __rhs_critical_enter(void) {
    __RHSCriticalInfo info;

    info.isrm = 0;
    info.from_isr = RHS_IS_ISR();
    info.kernel_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);

    if(info.from_isr) {
        info.isrm = taskENTER_CRITICAL_FROM_ISR();
    } else if(info.kernel_running) {
        taskENTER_CRITICAL();
    } else {
        __disable_irq();
    }

    return info;
}

void __rhs_critical_exit(__RHSCriticalInfo info) {
    if(info.from_isr) {
        taskEXIT_CRITICAL_FROM_ISR(info.isrm);
    } else if(info.kernel_running) {
        taskEXIT_CRITICAL();
    } else {
        __enable_irq();
    }
}
