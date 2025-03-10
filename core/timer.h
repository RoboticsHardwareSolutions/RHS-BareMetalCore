/**
 * @file timer.h
 * @brief RHS software Timer API.
 */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*RHSTimerCallback)(void* context);

typedef enum
{
    RHSTimerTypeOnce     = 0,  ///< One-shot timer.
    RHSTimerTypePeriodic = 1   ///< Repeating timer.
} RHSTimerType;

typedef struct RHSTimer RHSTimer;

/** Allocate timer
 *
 * @param[in]  func     The callback function
 * @param[in]  type     The timer type
 * @param      context  The callback context
 *
 * @return     The pointer to RHSTimer instance
 */
RHSTimer* rhs_timer_alloc(RHSTimerCallback func, RHSTimerType type, void* context);

/** Free timer
 *
 * @param      instance  The pointer to RHSTimer instance
 */
void rhs_timer_free(RHSTimer* instance);

/** Start timer
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to RHSTimer instance
 * @param[in]  ticks     The interval in ticks
 *
 * @return     The rhs status.
 */
RHSStatus rhs_timer_start(RHSTimer* instance, uint32_t ticks);

/** Restart timer with previous timeout value
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to RHSTimer instance
 * @param[in]  ticks     The interval in ticks
 *
 * @return     The rhs status.
 */
RHSStatus rhs_timer_restart(RHSTimer* instance, uint32_t ticks);

/** Stop timer
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to RHSTimer instance
 *
 * @return     The rhs status.
 */
RHSStatus rhs_timer_stop(RHSTimer* instance);

/** Is timer running
 *
 * @warning    This cal may and will return obsolete timer state if timer
 *             commands are still in the queue. Please read FreeRTOS timer
 *             documentation first.
 *
 * @param      instance  The pointer to RHSTimer instance
 *
 * @return     0: not running, 1: running
 */
uint32_t rhs_timer_is_running(RHSTimer* instance);

/** Get timer expire time
 *
 * @param      instance  The Timer instance
 *
 * @return     expire tick
 */
uint32_t rhs_timer_get_expire_time(RHSTimer* instance);

typedef void (*RHSTimerPendigCallback)(void* context, uint32_t arg);

void rhs_timer_pending_callback(RHSTimerPendigCallback callback, void* context, uint32_t arg);

typedef enum
{
    RHSTimerThreadPriorityNormal,   /**< Lower then other threads */
    RHSTimerThreadPriorityElevated, /**< Same as other threads */
} RHSTimerThreadPriority;

/** Set Timer thread priority
 *
 * @param[in]  priority  The priority
 */
void rhs_timer_set_thread_priority(RHSTimerThreadPriority priority);

#ifdef __cplusplus
}
#endif
