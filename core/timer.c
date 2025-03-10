#include "timer.h"
#include "kernel.h"
#include "common.h"
#include "memmgr.h"
#include "check.h"

#include <FreeRTOS.h>
#include <event_groups.h>
#include <timers.h>

struct RHSTimer
{
    StaticTimer_t     container;
    RHSTimerCallback cb_func;
    void*             cb_context;
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSTimer, container) == 0);

#define TIMER_DELETED_EVENT (1U << 0)

static void TimerCallback(TimerHandle_t hTimer)
{
    RHSTimer* instance = pvTimerGetTimerID(hTimer);
    rhs_assert(instance);
    instance->cb_func(instance->cb_context);
}

RHSTimer* rhs_timer_alloc(RHSTimerCallback func, RHSTimerType type, void* context)
{
    rhs_assert((rhs_kernel_is_irq_or_masked() == 0U) && (func != NULL));

    RHSTimer* instance = malloc(sizeof(RHSTimer));

    instance->cb_func    = func;
    instance->cb_context = context;

    const UBaseType_t   reload = (type == RHSTimerTypeOnce ? pdFALSE : pdTRUE);
    const TimerHandle_t hTimer =
        xTimerCreateStatic(NULL, portMAX_DELAY, reload, instance, TimerCallback, &instance->container);

    rhs_assert(hTimer == (TimerHandle_t) instance);

    return instance;
}

static void rhs_timer_epilogue(void* context, uint32_t arg)
{
    rhs_assert(context);
    (void) arg;

    EventGroupHandle_t hEvent = context;
    vTaskSuspendAll();
    xEventGroupSetBits(hEvent, TIMER_DELETED_EVENT);
    (void) xTaskResumeAll();
}

void rhs_timer_free(RHSTimer* instance)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t) instance;
    rhs_assert(xTimerDelete(hTimer, portMAX_DELAY) == pdPASS);

    StaticEventGroup_t event_container = {};
    EventGroupHandle_t hEvent          = xEventGroupCreateStatic(&event_container);
    rhs_assert(xTimerPendFunctionCall(rhs_timer_epilogue, hEvent, 0, portMAX_DELAY) == pdPASS);

    rhs_assert(xEventGroupWaitBits(hEvent, TIMER_DELETED_EVENT, pdFALSE, pdTRUE, portMAX_DELAY) ==
                 TIMER_DELETED_EVENT);
    vEventGroupDelete(hEvent);

    free(instance);
}

RHSStatus rhs_timer_start(RHSTimer* instance, uint32_t ticks)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);
    rhs_assert(ticks < portMAX_DELAY);

    TimerHandle_t hTimer = (TimerHandle_t) instance;
    RHSStatus    stat;

    if (xTimerChangePeriod(hTimer, ticks, portMAX_DELAY) == pdPASS)
    {
        stat = RHSStatusOk;
    }
    else
    {
        stat = RHSStatusErrorResource;
    }

    return stat;
}

RHSStatus rhs_timer_restart(RHSTimer* instance, uint32_t ticks)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);
    rhs_assert(ticks < portMAX_DELAY);

    TimerHandle_t hTimer = (TimerHandle_t) instance;
    RHSStatus    stat;

    if (xTimerChangePeriod(hTimer, ticks, portMAX_DELAY) == pdPASS && xTimerReset(hTimer, portMAX_DELAY) == pdPASS)
    {
        stat = RHSStatusOk;
    }
    else
    {
        stat = RHSStatusErrorResource;
    }

    return stat;
}

RHSStatus rhs_timer_stop(RHSTimer* instance)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t) instance;

    rhs_assert(xTimerStop(hTimer, portMAX_DELAY) == pdPASS);

    return RHSStatusOk;
}

uint32_t rhs_timer_is_running(RHSTimer* instance)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t) instance;

    /* Return 0: not running, 1: running */
    return (uint32_t) xTimerIsTimerActive(hTimer);
}

uint32_t rhs_timer_get_expire_time(RHSTimer* instance)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());
    rhs_assert(instance);

    TimerHandle_t hTimer = (TimerHandle_t) instance;

    return (uint32_t) xTimerGetExpiryTime(hTimer);
}

void rhs_timer_pending_callback(RHSTimerPendigCallback callback, void* context, uint32_t arg)
{
    rhs_assert(callback);

    BaseType_t ret = pdFAIL;
    if (rhs_kernel_is_irq_or_masked())
    {
        ret = xTimerPendFunctionCallFromISR(callback, context, arg, NULL);
    }
    else
    {
        ret = xTimerPendFunctionCall(callback, context, arg, RHSWaitForever);
    }

    rhs_assert(ret == pdPASS);
}

void rhs_timer_set_thread_priority(RHSTimerThreadPriority priority)
{
    rhs_assert(!rhs_kernel_is_irq_or_masked());

    TaskHandle_t task_handle = xTimerGetTimerDaemonTaskHandle();
    rhs_assert(task_handle);  // Don't call this method before timer task start

    if (priority == RHSTimerThreadPriorityNormal)
    {
        vTaskPrioritySet(task_handle, configTIMER_TASK_PRIORITY);
    }
    else if (priority == RHSTimerThreadPriorityElevated)
    {
        vTaskPrioritySet(task_handle, configMAX_PRIORITIES - 1);
    }
    else
    {
        rhs_assert(0);
    }
}
