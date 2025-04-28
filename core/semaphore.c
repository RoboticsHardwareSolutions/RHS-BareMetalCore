#include "semaphore.h"

#include <FreeRTOS.h>
#include <semphr.h>

#include "common.h"
#include "memmgr.h"
#include "check.h"
#include "kernel.h"

// Internal FreeRTOS member names
#define uxMessagesWaiting uxDummy4[0]
#define uxLength          uxDummy4[1]

struct RHSSemaphore {
    StaticSemaphore_t container;
    // RHSEventLoopLink event_loop_link;
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSSemaphore, container) == 0);

RHSSemaphore* rhs_semaphore_alloc(uint32_t max_count, uint32_t initial_count) {
    rhs_assert(!RHS_IS_IRQ_MODE());
    rhs_assert((max_count > 0U) && (initial_count <= max_count));

    RHSSemaphore* instance = malloc(sizeof(RHSSemaphore));

    SemaphoreHandle_t hSemaphore;

    if(max_count == 1U) {
        hSemaphore = xSemaphoreCreateBinaryStatic(&instance->container);
    } else {
        hSemaphore =
            xSemaphoreCreateCountingStatic(max_count, initial_count, &instance->container);
    }

    rhs_assert(hSemaphore == (SemaphoreHandle_t)instance);

    if(max_count == 1U && initial_count != 0U) {
        rhs_assert(xSemaphoreGive(hSemaphore) == pdPASS);
    }

    return instance;
}

void rhs_semaphore_free(RHSSemaphore* instance) {
    rhs_assert(instance);
    rhs_assert(!RHS_IS_IRQ_MODE());
    vSemaphoreDelete((SemaphoreHandle_t)instance);
    free(instance);
}

RHSStatus rhs_semaphore_acquire(RHSSemaphore* instance, uint32_t timeout) {
    rhs_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    RHSStatus stat;
    BaseType_t yield;

    stat = RHSStatusOk;

    if(RHS_IS_IRQ_MODE()) {
        if(timeout != 0U) {
            stat = RHSStatusErrorParameter;
        } else {
            yield = pdFALSE;

            if(xSemaphoreTakeFromISR(hSemaphore, &yield) != pdPASS) {
                stat = RHSStatusErrorResource;
            } else {
                portYIELD_FROM_ISR(yield);
            }
        }

    } else {
        if(xSemaphoreTake(hSemaphore, (TickType_t)timeout) != pdPASS) {
            if(timeout != 0U) {
                stat = RHSStatusErrorTimeout;
            } else {
                stat = RHSStatusErrorResource;
            }
        }
    }

    return stat;
}

RHSStatus rhs_semaphore_release(RHSSemaphore* instance) {
    rhs_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    RHSStatus stat;
    BaseType_t yield;

    stat = RHSStatusOk;

    if(RHS_IS_IRQ_MODE()) {
        yield = pdFALSE;

        if(xSemaphoreGiveFromISR(hSemaphore, &yield) != pdTRUE) {
            stat = RHSStatusErrorResource;
        } else {
            portYIELD_FROM_ISR(yield);
        }

    } else {
        if(xSemaphoreGive(hSemaphore) != pdPASS) {
            stat = RHSStatusErrorResource;
        }
    }

    return stat;
}

uint32_t rhs_semaphore_get_count(RHSSemaphore* instance) {
    rhs_assert(instance);

    SemaphoreHandle_t hSemaphore = (SemaphoreHandle_t)instance;
    uint32_t count;

    if(RHS_IS_IRQ_MODE()) {
        count = (uint32_t)uxSemaphoreGetCountFromISR(hSemaphore);
    } else {
        count = (uint32_t)uxSemaphoreGetCount(hSemaphore);
    }

    return count;
}

uint32_t rhs_semaphore_get_space(RHSSemaphore* instance) {
    rhs_assert(instance);

    uint32_t space;

    if(rhs_kernel_is_irq_or_masked() != 0U) {
        uint32_t isrm = taskENTER_CRITICAL_FROM_ISR();

        space = instance->container.uxLength - instance->container.uxMessagesWaiting;

        taskEXIT_CRITICAL_FROM_ISR(isrm);
    } else {
        space = uxQueueSpacesAvailable((QueueHandle_t)instance);
    }

    return space;
}
