/**
 * @file semaphore.h
 * RHSSemaphore
 */
#pragma once

#include "base.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHSSemaphore RHSSemaphore;

/** Allocate semaphore
 *
 * @param[in]  max_count      The maximum count
 * @param[in]  initial_count  The initial count
 *
 * @return     pointer to RHSSemaphore instance
 */
RHSSemaphore* rhs_semaphore_alloc(uint32_t max_count, uint32_t initial_count);

/** Free semaphore
 *
 * @param      instance  The pointer to RHSSemaphore instance
 */
void rhs_semaphore_free(RHSSemaphore* instance);

/** Acquire semaphore
 *
 * @param      instance  The pointer to RHSSemaphore instance
 * @param[in]  timeout   The timeout
 *
 * @return     The rhs status.
 */
RHSStatus rhs_semaphore_acquire(RHSSemaphore* instance, uint32_t timeout);

/** Release semaphore
 *
 * @param      instance  The pointer to RHSSemaphore instance
 *
 * @return     The rhs status.
 */
RHSStatus rhs_semaphore_release(RHSSemaphore* instance);

/** Get semaphore count
 *
 * @param      instance  The pointer to RHSSemaphore instance
 *
 * @return     Semaphore count
 */
uint32_t rhs_semaphore_get_count(RHSSemaphore* instance);

/** Get available space
 *
 * @param      instance  The pointer to RHSSemaphore instance
 *
 * @return     Semaphore available space
 */
uint32_t rhs_semaphore_get_space(RHSSemaphore* instance);

#ifdef __cplusplus
}
#endif
