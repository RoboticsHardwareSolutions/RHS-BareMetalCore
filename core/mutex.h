#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <FreeRTOS.h>
#include <semphr.h>
#include "base.h"


typedef enum {
    RHSMutexTypeNormal,
    RHSMutexTypeRecursive,
} RHSMutexType;

typedef struct RHSMutex RHSMutex;

/** Allocate RHSMutex
 *
 * @param[in]  type  The mutex type
 *
 * @return     pointer to RHSMutex instance
 */
RHSMutex* rhs_mutex_alloc(RHSMutexType type);

/** Free RHSMutex
 *
 * @param      instance  The pointer to RHSMutex instance
 */
void rhs_mutex_free(RHSMutex* instance);

/** Acquire mutex
 *
 * @param      instance  The pointer to RHSMutex instance
 * @param[in]  timeout   The timeout
 *
 * @return     The rhs status.
 */
RHSStatus rhs_mutex_acquire(RHSMutex* instance, uint32_t timeout);

/** Release mutex
 *
 * @param      instance  The pointer to RHSMutex instance
 *
 * @return     The rhs status.
 */
RHSStatus rhs_mutex_release(RHSMutex* instance);

/** Get mutex owner thread id
 *
 * @param      instance  The pointer to RHSMutex instance
 *
 * @return     The rhs thread identifier.
 */
TaskHandle_t rhs_mutex_get_owner(RHSMutex* instance);

#ifdef __cplusplus
}
#endif
