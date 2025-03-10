#include "message_queue.h"

#include <FreeRTOS.h>
#include <queue.h>

#include "kernel.h"
#include "memmgr.h"
#include "common.h"
#include "check.h"

// Internal FreeRTOS member names
#define uxMessagesWaiting uxDummy4[0]
#define uxLength uxDummy4[1]
#define uxItemSize uxDummy4[2]

struct RHSMessageQueue
{
    StaticQueue_t container;
    uint8_t       buffer[];
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSMessageQueue, container) == 0, "");
// IMPORTANT: buffer MUST be the LAST struct member
static_assert(offsetof(RHSMessageQueue, buffer) == sizeof(RHSMessageQueue), "");

RHSMessageQueue* rhs_message_queue_alloc(uint32_t msg_count, uint32_t msg_size)
{
    rhs_assert((rhs_kernel_is_irq_or_masked() == 0U) && (msg_count > 0U) && (msg_size > 0U));

    RHSMessageQueue* instance = malloc(sizeof(RHSMessageQueue) + msg_count * msg_size);

    // 3 things happens here:
    // - create queue
    // - check results
    // - ensure that queue container is first in the RHSMessageQueue structure
    //
    // As a bonus it guarantees that RHSMessageQueue* can be casted into StaticQueue_t* or QueueHandle_t.
    rhs_assert(xQueueCreateStatic(msg_count, msg_size, instance->buffer, &instance->container) == (void*) instance);
    return instance;
}

void rhs_message_queue_free(RHSMessageQueue* instance)
{
    rhs_assert(rhs_kernel_is_irq_or_masked() == 0U);
    rhs_assert(instance);

    vQueueDelete((QueueHandle_t) instance);
    free(instance);
}

RHSStatus rhs_message_queue_put(RHSMessageQueue* instance, const void* msg_ptr, uint32_t timeout)
{
    rhs_assert(instance);

    QueueHandle_t hQueue = (QueueHandle_t) instance;
    RHSStatus    stat;
    BaseType_t    yield;

    stat = RHSStatusOk;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        if ((msg_ptr == NULL) || (timeout != 0U))
        {
            stat = RHSStatusErrorParameter;
        }
        else
        {
            yield = pdFALSE;

            if (xQueueSendToBackFromISR(hQueue, msg_ptr, &yield) != pdTRUE)
            {
                stat = RHSStatusErrorResource;
            }
            else
            {
                portYIELD_FROM_ISR(yield);
            }
        }
    }
    else
    {
        if (msg_ptr == NULL)
        {
            stat = RHSStatusErrorParameter;
        }
        else
        {
            if (xQueueSendToBack(hQueue, msg_ptr, (TickType_t) timeout) != pdPASS)
            {
                if (timeout != 0U)
                {
                    stat = RHSStatusErrorTimeout;
                }
                else
                {
                    stat = RHSStatusErrorResource;
                }
            }
        }
    }

    /* Return execution status */
    return stat;
}

RHSStatus rhs_message_queue_get(RHSMessageQueue* instance, void* msg_ptr, uint32_t timeout)
{
    rhs_assert(instance);

    QueueHandle_t hQueue = (QueueHandle_t) instance;
    RHSStatus    stat;
    BaseType_t    yield;

    stat = RHSStatusOk;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        if ((msg_ptr == NULL) || (timeout != 0U))
        {
            stat = RHSStatusErrorParameter;
        }
        else
        {
            yield = pdFALSE;

            if (xQueueReceiveFromISR(hQueue, msg_ptr, &yield) != pdPASS)
            {
                stat = RHSStatusErrorResource;
            }
            else
            {
                portYIELD_FROM_ISR(yield);
            }
        }
    }
    else
    {
        if (msg_ptr == NULL)
        {
            stat = RHSStatusErrorParameter;
        }
        else
        {
            if (xQueueReceive(hQueue, msg_ptr, (TickType_t) timeout) != pdPASS)
            {
                if (timeout != 0U)
                {
                    stat = RHSStatusErrorTimeout;
                }
                else
                {
                    stat = RHSStatusErrorResource;
                }
            }
        }
    }

    return stat;
}

uint32_t rhs_message_queue_get_capacity(RHSMessageQueue* instance)
{
    rhs_assert(instance);

    return instance->container.uxLength;
}

uint32_t rhs_message_queue_get_message_size(RHSMessageQueue* instance)
{
    rhs_assert(instance);

    return instance->container.uxItemSize;
}

uint32_t rhs_message_queue_get_count(RHSMessageQueue* instance)
{
    rhs_assert(instance);

    QueueHandle_t hQueue = (QueueHandle_t) instance;
    UBaseType_t   count;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        count = uxQueueMessagesWaitingFromISR(hQueue);
    }
    else
    {
        count = uxQueueMessagesWaiting(hQueue);
    }

    return (uint32_t) count;
}

uint32_t rhs_message_queue_get_space(RHSMessageQueue* instance)
{
    rhs_assert(instance);

    uint32_t space;
    uint32_t isrm;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        isrm = taskENTER_CRITICAL_FROM_ISR();

        space = instance->container.uxLength - instance->container.uxMessagesWaiting;

        taskEXIT_CRITICAL_FROM_ISR(isrm);
    }
    else
    {
        space = (uint32_t) uxQueueSpacesAvailable((QueueHandle_t) instance);
    }

    return space;
}

RHSStatus rhs_message_queue_reset(RHSMessageQueue* instance)
{
    rhs_assert(instance);

    QueueHandle_t hQueue = (QueueHandle_t) instance;
    RHSStatus    stat;

    if (rhs_kernel_is_irq_or_masked() != 0U)
    {
        stat = RHSStatusErrorISR;
    }
    else
    {
        stat = RHSStatusOk;
        (void) xQueueReset(hQueue);
    }

    /* Return execution status */
    return stat;
}
