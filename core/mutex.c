#include "mutex.h"
#include "common.h"
#include "memmgr.h"
#include "check.h"


// Internal FreeRTOS member names
#define ucQueueType ucDummy9

struct RHSMutex
{
    StaticSemaphore_t container;
};

// IMPORTANT: container MUST be the FIRST struct member
static_assert(offsetof(RHSMutex, container) == 0, "");

RHSMutex* rhs_mutex_alloc(RHSMutexType type) {
    rhs_assert(!RHS_IS_IRQ_MODE());

    RHSMutex* instance = malloc(sizeof(RHSMutex));

    SemaphoreHandle_t hMutex;

    if(type == RHSMutexTypeNormal) {
        hMutex = xSemaphoreCreateMutexStatic(&instance->container);
    } else if(type == RHSMutexTypeRecursive) {
        hMutex = xSemaphoreCreateRecursiveMutexStatic(&instance->container);
    } else {
        rhs_assert(0);
    }

    rhs_assert(hMutex == (SemaphoreHandle_t)instance);

    return instance;
}

void rhs_mutex_free(RHSMutex* instance) {
    rhs_assert(!RHS_IS_IRQ_MODE());
    rhs_assert(instance);

    vSemaphoreDelete((SemaphoreHandle_t)instance);
    free(instance);
}

RHSStatus rhs_mutex_acquire(RHSMutex* instance, uint32_t timeout) {
    rhs_assert(instance);

    SemaphoreHandle_t hMutex = (SemaphoreHandle_t)(instance);
    const uint8_t mutex_type = instance->container.ucQueueType;

    RHSStatus stat = RHSStatusOk;

    if(RHS_IS_IRQ_MODE()) {
        stat = RHSStatusErrorISR;

    } else if(mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
        if(xSemaphoreTakeRecursive(hMutex, timeout) != pdPASS) {
            if(timeout != 0U) {
                stat = RHSStatusErrorTimeout;
            } else {
                stat = RHSStatusErrorResource;
            }
        }

    } else if(mutex_type == queueQUEUE_TYPE_MUTEX) {
        if(xSemaphoreTake(hMutex, timeout) != pdPASS) {
            if(timeout != 0U) {
                stat = RHSStatusErrorTimeout;
            } else {
                stat = RHSStatusErrorResource;
            }
        }

    } else {
        rhs_assert(0);
    }

    return stat;
}

RHSStatus rhs_mutex_release(RHSMutex* instance) {
    rhs_assert(instance);

    SemaphoreHandle_t hMutex = (SemaphoreHandle_t)(instance);
    const uint8_t mutex_type = instance->container.ucQueueType;

    RHSStatus status = RHSStatusOk;

    if(RHS_IS_IRQ_MODE()) {
        status = RHSStatusErrorISR;

    } else if(mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
        if(xSemaphoreGiveRecursive(hMutex) != pdPASS) {
            status = RHSStatusErrorResource;
        }

    } else if(mutex_type == queueQUEUE_TYPE_MUTEX) {
        if(xSemaphoreGive(hMutex) != pdPASS) {
            status = RHSStatusErrorResource;
        }

    } else {
        rhs_assert(0);
    }

    return status;
}

TaskHandle_t rhs_mutex_get_owner(RHSMutex* instance) {
    rhs_assert(instance);

    SemaphoreHandle_t hMutex = (SemaphoreHandle_t)instance;

    TaskHandle_t owner;

    if(RHS_IS_IRQ_MODE()) {
        owner = xSemaphoreGetMutexHolderFromISR(hMutex);
    } else {
        owner = xSemaphoreGetMutexHolder(hMutex);
    }

    return owner;
}
