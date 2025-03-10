#include "event_flag.h"
#include "common.h"
#include "memmgr.h"
#include "check.h"

#include <FreeRTOS.h>
#include <event_groups.h>

#define RHS_EVENT_FLAG_MAX_BITS_EVENT_GROUPS 24U
#define RHS_EVENT_FLAG_INVALID_BITS (~((1UL << RHS_EVENT_FLAG_MAX_BITS_EVENT_GROUPS) - 1U))

struct RHSEventFlag
{
    StaticEventGroup_t container;
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSEventFlag, container) == 0, "");

RHSEventFlag* rhs_event_flag_alloc(void)
{
    rhs_assert(!RHS_IS_IRQ_MODE());

    RHSEventFlag* instance = malloc(sizeof(RHSEventFlag));

    rhs_assert(xEventGroupCreateStatic(&instance->container) == (EventGroupHandle_t) instance);

    return instance;
}

void rhs_event_flag_free(RHSEventFlag* instance)
{
    rhs_assert(!RHS_IS_IRQ_MODE());
    vEventGroupDelete((EventGroupHandle_t) instance);
    free(instance);
}

uint32_t rhs_event_flag_set(RHSEventFlag* instance, uint32_t flags)
{
    rhs_assert(instance);
    rhs_assert((flags & RHS_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t) instance;
    uint32_t           rflags;
    BaseType_t         yield;

    if (RHS_IS_IRQ_MODE())
    {
        yield = pdFALSE;
        if (xEventGroupSetBitsFromISR(hEventGroup, (EventBits_t) flags, &yield) == pdFAIL)
        {
            rflags = (uint32_t) RHSFlagErrorResource;
        }
        else
        {
            rflags = flags;
            portYIELD_FROM_ISR(yield);
        }
    }
    else
    {
        vTaskSuspendAll();
        rflags = xEventGroupSetBits(hEventGroup, (EventBits_t) flags);
        (void) xTaskResumeAll();
    }

    /* Return event flags after setting */
    return rflags;
}

uint32_t rhs_event_flag_clear(RHSEventFlag* instance, uint32_t flags)
{
    rhs_assert(instance);
    rhs_assert((flags & RHS_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t) instance;
    uint32_t           rflags;

    if (RHS_IS_IRQ_MODE())
    {
        rflags = xEventGroupGetBitsFromISR(hEventGroup);

        if (xEventGroupClearBitsFromISR(hEventGroup, (EventBits_t) flags) == pdFAIL)
        {
            rflags = (uint32_t) RHSStatusErrorResource;
        }
        else
        {
            /* xEventGroupClearBitsFromISR only registers clear operation in the timer command queue. */
            /* Yield is required here otherwise clear operation might not execute in the right order. */
            /* See https://github.com/FreeRTOS/FreeRTOS-Kernel/issues/93 for more info.               */
            portYIELD_FROM_ISR(pdTRUE);
        }
    }
    else
    {
        rflags = xEventGroupClearBits(hEventGroup, (EventBits_t) flags);
    }

    /* Return event flags before clearing */
    return rflags;
}

uint32_t rhs_event_flag_get(RHSEventFlag* instance)
{
    rhs_assert(instance);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t) instance;
    uint32_t           rflags;

    if (RHS_IS_IRQ_MODE())
    {
        rflags = xEventGroupGetBitsFromISR(hEventGroup);
    }
    else
    {
        rflags = xEventGroupGetBits(hEventGroup);
    }

    /* Return current event flags */
    return rflags;
}

uint32_t rhs_event_flag_wait(RHSEventFlag* instance, uint32_t flags, uint32_t options, uint32_t timeout)
{
    rhs_assert(!RHS_IS_IRQ_MODE());
    rhs_assert(instance);
    rhs_assert((flags & RHS_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t) instance;
    BaseType_t         wait_all;
    BaseType_t         exit_clr;
    uint32_t           rflags;

    if (options & RHSFlagWaitAll)
    {
        wait_all = pdTRUE;
    }
    else
    {
        wait_all = pdFAIL;
    }

    if (options & RHSFlagNoClear)
    {
        exit_clr = pdFAIL;
    }
    else
    {
        exit_clr = pdTRUE;
    }

    rflags = xEventGroupWaitBits(hEventGroup, (EventBits_t) flags, exit_clr, wait_all, (TickType_t) timeout);

    if (options & RHSFlagWaitAll)
    {
        if ((flags & rflags) != flags)
        {
            if (timeout > 0U)
            {
                rflags = (uint32_t) RHSStatusErrorTimeout;
            }
            else
            {
                rflags = (uint32_t) RHSStatusErrorResource;
            }
        }
    }
    else
    {
        if ((flags & rflags) == 0U)
        {
            if (timeout > 0U)
            {
                rflags = (uint32_t) RHSStatusErrorTimeout;
            }
            else
            {
                rflags = (uint32_t) RHSStatusErrorResource;
            }
        }
    }

    /* Return event flags before clearing */
    return rflags;
}
