#include "kernel.h"
#include "base.h"
#include "common.h"
#include "rhs_hal_cortex.h"

#include <FreeRTOS.h>
#include <task.h>

bool rhs_kernel_is_irq_or_masked(void)
{
    bool       irq = false;
    BaseType_t state;

    if (RHS_IS_IRQ_MODE())
    {
        /* Called from interrupt context */
        irq = true;
    }
    else
    {
        /* Get FreeRTOS scheduler state */
        state = xTaskGetSchedulerState();

        if (state != taskSCHEDULER_NOT_STARTED)
        {
            /* Scheduler was started */
            if (RHS_IS_IRQ_MASKED())
            {
                /* Interrupts are masked */
                irq = true;
            }
        }
    }

    /* Return context, 0: thread context, 1: IRQ context */
    return irq;
}

bool rhs_kernel_is_running(void)
{
    return xTaskGetSchedulerState() == taskSCHEDULER_RUNNING;
}

int32_t rhs_kernel_lock(void)
{
    configASSERT(!rhs_kernel_is_irq_or_masked());

    int32_t lock;

    switch (xTaskGetSchedulerState())
    {
    case taskSCHEDULER_SUSPENDED:
        lock = 1;
        break;

    case taskSCHEDULER_RUNNING:
        vTaskSuspendAll();
        lock = 0;
        break;

    case taskSCHEDULER_NOT_STARTED:
    default:
        lock = (int32_t) RHSStatusError;
        break;
    }

    /* Return previous lock state */
    return lock;
}

int32_t rhs_kernel_unlock(void)
{
    configASSERT(!rhs_kernel_is_irq_or_masked());

    int32_t lock;

    switch (xTaskGetSchedulerState())
    {
    case taskSCHEDULER_SUSPENDED:
        lock = 1;

        if (xTaskResumeAll() != pdTRUE)
        {
            if (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED)
            {
                lock = (int32_t) RHSStatusError;
            }
        }
        break;

    case taskSCHEDULER_RUNNING:
        lock = 0;
        break;

    case taskSCHEDULER_NOT_STARTED:
    default:
        lock = (int32_t) RHSStatusError;
        break;
    }

    /* Return previous lock state */
    return lock;
}

int32_t rhs_kernel_restore_lock(int32_t lock)
{
    configASSERT(!rhs_kernel_is_irq_or_masked());

    switch (xTaskGetSchedulerState())
    {
    case taskSCHEDULER_SUSPENDED:
    case taskSCHEDULER_RUNNING:
        if (lock == 1)
        {
            vTaskSuspendAll();
        }
        else
        {
            if (lock != 0)
            {
                lock = (int32_t) RHSStatusError;
            }
            else
            {
                if (xTaskResumeAll() != pdTRUE)
                {
                    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
                    {
                        lock = (int32_t) RHSStatusError;
                    }
                }
            }
        }
        break;

    case taskSCHEDULER_NOT_STARTED:
    default:
        lock = (int32_t) RHSStatusError;
        break;
    }

    /* Return new lock state */
    return lock;
}

uint32_t rhs_kernel_get_tick_frequency(void)
{
    /* Return frequency in hertz */
    return configTICK_RATE_HZ;
}

void rhs_delay_tick(uint32_t ticks)
{
    configASSERT(!rhs_kernel_is_irq_or_masked());
    if (ticks == 0U)
    {
        taskYIELD();
    }
    else
    {
        vTaskDelay(ticks);
    }
}

RHSStatus rhs_delay_until_tick(uint32_t tick)
{
    configASSERT(!rhs_kernel_is_irq_or_masked());

    TickType_t tcnt, delay;
    RHSStatus stat;

    stat = RHSStatusOk;
    tcnt = xTaskGetTickCount();

    /* Determine remaining number of tick to delay */
    delay = (TickType_t) tick - tcnt;

    /* Check if target tick has not expired */
    if ((delay != 0U) && (0 == (delay >> (8 * sizeof(TickType_t) - 1))))
    {
        vTaskDelayUntil(&tcnt, delay);
    }
    else
    {
        /* No delay or already expired */
        stat = RHSStatusErrorParameter;
    }

    /* Return execution status */
    return stat;
}

uint32_t rhs_get_tick(void)
{
    TickType_t ticks;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        ticks = xTaskGetTickCountFromISR();
    }
    else
    {
        ticks = xTaskGetTickCount();
    }

    return ticks;
}

uint32_t rhs_ms_to_ticks(uint32_t milliseconds)
{
    if (configTICK_RATE_HZ == 1000)
        return milliseconds;
    return (uint32_t) ((float) configTICK_RATE_HZ) / 1000.0f * (float) milliseconds;
}

void rhs_delay_ms(uint32_t milliseconds)
{
    if (!RHS_IS_ISR() && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        if (milliseconds > 0 && milliseconds < portMAX_DELAY - 1)
        {
            milliseconds += 1;
        }
#if configTICK_RATE_HZ_RAW == 1000
        rhs_delay_tick(milliseconds);
#else
        rhs_delay_tick(rhs_ms_to_ticks(milliseconds));
#endif
    }
    else if (milliseconds > 0)
    {
        rhs_delay_us(milliseconds * 1000);
    }
}

void rhs_delay_us(uint32_t microseconds)
{
    rhs_hal_cortex_delay_us(microseconds);
}
