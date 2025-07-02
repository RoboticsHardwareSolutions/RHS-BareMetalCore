#include <stdbool.h>
#include "memmgr.h"
#include "thread.h"
#include "check.h"
#include "log.h"
#include "common.h"
#include "kernel.h"
#include <FreeRTOS.h>
#include "task.h"
#include "message_queue.h"
#include <stdint.h>
#include "thread_list.h"

#define TAG "thread"

#define THREAD_NOTIFY_INDEX (1)  // Index 0 is used for stream buffers

struct RHSThread
{
    StaticTask_t container;
    StackType_t* stack_buffer;

    volatile RHSThreadState state;
    int32_t                 ret;

    RHSThreadCallback callback;
    void*             context;

    //    RHSThreadStateCallback state_callback;
    //    void* state_context;
    //
    //    RHSThreadSignalCallback signal_callback;
    //    void* signal_context;

    char* name;
    char* appid;

    RHSThreadPriority priority;

    size_t stack_size;
    size_t heap_size;

    //    RHSThreadStdout output;
    //    RHSThreadStdin input;

    // Keep all non-alignable byte types in one place,
    // this ensures that the size of this structure is minimal
    bool is_service;
    bool heap_trace_enabled;
};

static RHSMessageQueue* rhs_thread_scrub_message_queue = NULL;

/** Catch threads that are trying to exit wrong way */
__attribute__((__noreturn__)) void rhs_thread_catch(void)
{  //-V1082
    // If you're here it means you're probably doing something wrong
    // with critical sections or with scheduler state
    asm volatile("nop");                  // extra magic
    rhs_crash("You are doing it wrong");  //-V779
    __builtin_unreachable();
}

static void rhs_thread_set_state(RHSThread* thread, RHSThreadState state)
{
    rhs_assert(thread);
    thread->state = state;
}

static void rhs_thread_body(void* context)
{
    rhs_assert(context);
    RHSThread* thread = context;

    // store thread instance to thread local storage
    rhs_assert(pvTaskGetThreadLocalStoragePointer(NULL, 0) == NULL);
    vTaskSetThreadLocalStoragePointer(NULL, 0, thread);

    // TODO (p.firsov) check state of thread for not double call

    rhs_assert(thread->state == RHSThreadStateStarting);
    rhs_thread_set_state(thread, RHSThreadStateRunning);

    thread->ret = thread->callback(thread->context);
    rhs_assert(!thread->is_service);

    rhs_assert(thread->state == RHSThreadStateRunning);

    rhs_thread_set_state(thread, RHSThreadStateStopping);

    rhs_message_queue_put(rhs_thread_scrub_message_queue, &thread, RHSWaitForever);

    vTaskSuspend(NULL);
    rhs_thread_catch();
}

void rhs_thread_init(void)
{
    rhs_thread_scrub_message_queue = rhs_message_queue_alloc(8, sizeof(RHSThread*));
}

void rhs_thread_set_name(RHSThread* thread, const char* name)
{
    rhs_assert(thread);

    if (thread->name)
    {
        free(thread->name);
    }

    thread->name = name ? strdup(name) : NULL;
}

RHSThread* rhs_thread_alloc(const char* name, uint32_t stack_size, RHSThreadCallback callback, void* context)
{
    RHSThread* thread    = calloc(1, sizeof(RHSThread));
    thread->stack_buffer = malloc(stack_size);
    thread->stack_size   = stack_size;
    thread->callback     = callback;
    thread->context      = context;
    thread->priority     = RHSThreadPriorityNormal;
    thread->is_service   = false;
    rhs_thread_set_name(thread, name);
    return thread;
}

RHSThread* rhs_thread_alloc_ex(const char*       name,
                               uint32_t          stack_size,
                               RHSThreadPriority priotity,
                               RHSThreadCallback callback,
                               void*             context)
{
    RHSThread* thread    = rhs_thread_alloc(name, stack_size, callback, context);
    thread->priority     = priotity;
    return thread;
}

RHSThread* rhs_thread_alloc_service(const char* name, uint32_t stack_size, RHSThreadCallback callback, void* context)
{
    RHSThread* thread    = rhs_thread_alloc(name, stack_size, callback, context);
    thread->is_service   = true;
    return thread;
}

void rhs_thread_free(RHSThread* thread)
{
    rhs_assert(thread);
    rhs_assert(!thread->is_service);

    rhs_thread_set_name(thread, NULL);

    if (thread->stack_buffer)
    {
        free(thread->stack_buffer);
    }

    free(thread);
}

void rhs_thread_start(RHSThread* thread)
{
    rhs_assert(thread);
    rhs_assert(thread->callback);
    rhs_assert(thread->state == RHSThreadStateStopped);
    rhs_assert(thread->stack_size > 0);

    rhs_thread_set_state(thread, RHSThreadStateStarting);

    uint32_t stack_depth = thread->stack_size / sizeof(StackType_t);

    rhs_assert(xTaskCreateStatic(rhs_thread_body,
                                 thread->name,
                                 stack_depth,
                                 thread,
                                 thread->priority,
                                 thread->stack_buffer,
                                 &thread->container) == (TaskHandle_t) thread);
}

bool rhs_thread_join(RHSThread* thread)
{
    rhs_assert(thread);
    rhs_assert(!thread->is_service);
    // Cannot join a thread to itself
    rhs_assert(rhs_thread_get_current() != thread);

    // !!! IMPORTANT NOTICE !!!
    //
    // If your thread exited, but your app stuck here: some other thread uses
    // all cpu time, which delays kernel from releasing task handle
    while (thread->state != RHSThreadStateStopped)
    {
        rhs_delay_tick(2);
    }

    return true;
}

RHSThreadId rhs_thread_get_current_id(void)
{
    return (RHSThreadId) xTaskGetCurrentTaskHandle();
}

RHSThread* rhs_thread_get_current(void)
{
    RHSThread* thread = pvTaskGetThreadLocalStoragePointer(NULL, 0);
    return thread;
}

RHSThreadId rhs_thread_get_id(RHSThread* thread)
{
    rhs_assert(thread);
    return (RHSThreadId) thread;
}

/* Limits */
#define MAX_BITS_TASK_NOTIFY 31U
#define MAX_BITS_EVENT_GROUPS 24U

#define THREAD_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_TASK_NOTIFY) - 1U))
#define EVENT_FLAGS_INVALID_BITS (~((1UL << MAX_BITS_EVENT_GROUPS) - 1U))

uint32_t rhs_thread_flags_set(RHSThreadId thread_id, uint32_t flags)
{
    TaskHandle_t hTask = (TaskHandle_t) thread_id;
    uint32_t     rflags;
    BaseType_t   yield;

    if ((hTask == NULL) || ((flags & THREAD_FLAGS_INVALID_BITS) != 0U))
    {
        rflags = (uint32_t) RHSStatusErrorParameter;
    }
    else
    {
        rflags = (uint32_t) RHSStatusError;

        if (RHS_IS_IRQ_MODE())
        {
            yield = pdFALSE;

            (void) xTaskNotifyIndexedFromISR(hTask, THREAD_NOTIFY_INDEX, flags, eSetBits, &yield);
            (void) xTaskNotifyAndQueryIndexedFromISR(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags, NULL);

            portYIELD_FROM_ISR(yield);
        }
        else
        {
            (void) xTaskNotifyIndexed(hTask, THREAD_NOTIFY_INDEX, flags, eSetBits);
            (void) xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags);
        }
    }
    /* Return flags after setting */
    return rflags;
}

uint32_t rhs_thread_flags_clear(uint32_t flags)
{
    TaskHandle_t hTask;
    uint32_t     rflags, cflags;

    if (RHS_IS_IRQ_MODE())
    {
        rflags = (uint32_t) RHSStatusErrorISR;
    }
    else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)
    {
        rflags = (uint32_t) RHSStatusErrorParameter;
    }
    else
    {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &cflags) == pdPASS)
        {
            rflags = cflags;
            cflags &= ~flags;

            if (xTaskNotifyIndexed(hTask, THREAD_NOTIFY_INDEX, cflags, eSetValueWithOverwrite) != pdPASS)
            {
                rflags = (uint32_t) RHSStatusError;
            }
        }
        else
        {
            rflags = (uint32_t) RHSStatusError;
        }
    }

    /* Return flags before clearing */
    return rflags;
}

uint32_t rhs_thread_flags_get(void)
{
    TaskHandle_t hTask;
    uint32_t     rflags;

    if (RHS_IS_IRQ_MODE())
    {
        rflags = (uint32_t) RHSStatusErrorISR;
    }
    else
    {
        hTask = xTaskGetCurrentTaskHandle();

        if (xTaskNotifyAndQueryIndexed(hTask, THREAD_NOTIFY_INDEX, 0, eNoAction, &rflags) != pdPASS)
        {
            rflags = (uint32_t) RHSStatusError;
        }
    }

    return rflags;
}

uint32_t rhs_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout)
{
    uint32_t   rflags, nval;
    uint32_t   clear;
    TickType_t t0, td, tout;
    BaseType_t rval;

    if (RHS_IS_IRQ_MODE())
    {
        rflags = (uint32_t) RHSStatusErrorISR;
    }
    else if ((flags & THREAD_FLAGS_INVALID_BITS) != 0U)
    {
        rflags = (uint32_t) RHSStatusErrorParameter;
    }
    else
    {
        if ((options & RHSFlagNoClear) == RHSFlagNoClear)
        {
            clear = 0U;
        }
        else
        {
            clear = flags;
        }

        rflags = 0U;
        tout   = timeout;

        t0 = xTaskGetTickCount();
        do
        {
            rval = xTaskNotifyWaitIndexed(THREAD_NOTIFY_INDEX, 0, clear, &nval, tout);

            if (rval == pdPASS)
            {
                rflags &= flags;
                rflags |= nval;

                if ((options & RHSFlagWaitAll) == RHSFlagWaitAll)
                {
                    if ((flags & rflags) == flags)
                    {
                        break;
                    }
                    else
                    {
                        if (timeout == 0U)
                        {
                            rflags = (uint32_t) RHSStatusErrorResource;
                            break;
                        }
                    }
                }
                else
                {
                    if ((flags & rflags) != 0)
                    {
                        break;
                    }
                    else
                    {
                        if (timeout == 0U)
                        {
                            rflags = (uint32_t) RHSStatusErrorResource;
                            break;
                        }
                    }
                }

                /* Update timeout */
                td = xTaskGetTickCount() - t0;

                if (td > tout)
                {
                    tout = 0;
                }
                else
                {
                    tout -= td;
                }
            }
            else
            {
                if (timeout == 0)
                {
                    rflags = (uint32_t) RHSStatusErrorResource;
                }
                else
                {
                    rflags = (uint32_t) RHSStatusErrorTimeout;
                }
            }
        } while (rval != pdFAIL);
    }

    /* Return flags before clearing */
    return rflags;
}

void rhs_thread_scrub(void)
{
    RHSThread* thread_to_scrub = NULL;
    while (true)
    {
        rhs_assert(rhs_message_queue_get(rhs_thread_scrub_message_queue, &thread_to_scrub, RHSWaitForever) ==
                   RHSStatusOk);

        TaskHandle_t task = (TaskHandle_t) thread_to_scrub;

        // Delete task: FreeRTOS will remove task from all lists where it may be
        vTaskDelete(task);
        // Sanity check: ensure that local storage is ours and clear it
        rhs_assert(pvTaskGetThreadLocalStoragePointer(task, 0) == thread_to_scrub);
        vTaskSetThreadLocalStoragePointer(task, 0, NULL);

        // Deliver thread stopped callback
        rhs_thread_set_state(thread_to_scrub, RHSThreadStateStopped);
        RHS_LOG_D(TAG, "task deleted");
    }
}

static const char* rhs_thread_state_name(eTaskState state)
{
    switch (state)
    {
    case eRunning:
        return "Running";
    case eReady:
        return "Ready";
    case eBlocked:
        return "Blocked";
    case eSuspended:
        return "Suspended";
    case eDeleted:
        return "Deleted";
    case eInvalid:
        return "Invalid";
    default:
        return "?";
    }
}

void rhs_thread_enumerate(RHSThreadList* thread_list)
{
    rhs_assert(!RHS_IS_IRQ_MODE());

    vTaskSuspendAll();
    do
    {
        uint32_t tick  = rhs_get_tick();
        uint32_t count = uxTaskGetNumberOfTasks();

        TaskStatus_t* task = pvPortMalloc(count * sizeof(TaskStatus_t));

        if (!task)
            break;

        configRUN_TIME_COUNTER_TYPE total_run_time;
        count = uxTaskGetSystemState(task, count, &total_run_time);
        rhs_thread_list_erase(thread_list);

        for (uint32_t i = 0U; i < count; i++)
        {
            RHSThreadListItem* item = rhs_thread_list_add(thread_list);

            item->name             = task[i].pcTaskName;
            item->priority         = task[i].uxCurrentPriority;
            item->stack_address    = (uint32_t) task[i].pxStackBase;
            item->stack_size       = (task[i].pxEndOfStack - task[i].pxStackBase + 2);
            item->stack_min_free   = (uint32_t) (uxTaskGetStackHighWaterMark(task[i].xHandle));
            item->state            = rhs_thread_state_name(task[i].eCurrentState);
            item->counter_previous = item->counter_current;
            item->counter_current  = task[i].ulRunTimeCounter;
            item->tick             = tick;
        }
        vPortFree(task);
        rhs_thread_list_process(thread_list, total_run_time, tick);
    } while (false);
    (void) xTaskResumeAll();
}
