#pragma once
#include <stdbool.h>
#include "stdint.h"
/**
 * @brief Enumeration of possible RHSThread states.
 *
 * Many of the RHSThread functions MUST ONLY be called when the thread is STOPPED.
 */
typedef enum {
    RHSThreadStateStopped, /**< Thread is stopped and is safe to release. Event delivered from system init thread(TCB cleanup routine). It is safe to release thread instance. */
    RHSThreadStateStopping, /**< Thread is stopping. Event delivered from child thread. */
    RHSThreadStateStarting, /**< Thread is starting. Event delivered from parent(self) thread. */
    RHSThreadStateRunning, /**< Thread is running. Event delivered from child thread. */
} RHSThreadState;

/**
 * @brief Enumeration of possible RHSThread priorities.
 */
typedef enum {
    RHSThreadPriorityIdle = 0, /**< Idle priority */
    RHSThreadPriorityInit = 4, /**< Init System Thread Priority */
    RHSThreadPriorityLowest = 14, /**< Lowest */
    RHSThreadPriorityLow = 15, /**< Low */
    RHSThreadPriorityNormal = 16, /**< Normal, system default */
    RHSThreadPriorityHigh = 17, /**< High */
    RHSThreadPriorityHighest = 18, /**< Highest */
    RHSThreadPriorityIsr =
    (32 - 1), /**< Deferred ISR (highest possible) */
} RHSThreadPriority;

/**
 * @brief RHSThread opaque type.
 */
typedef struct RHSThread RHSThread;

typedef void* RHSThreadId;

/**
 * @brief Thread callback function pointer type.
 *
 * The function to be used as a thread callback MUST follow this signature.
 *
 * @param[in,out] context pointer to a user-specified object
 * @return value to be used as the thread return code
 */
typedef int32_t (*RHSThreadCallback)(void* context);

void rhs_thread_init(void);

/**
 * @brief Create a RHSThread instance (service mode).
 *
 * Service threads are more memory efficient, but have
 * the following limitations:
 *
 * - Cannot return from the callback
 * - Cannot be joined or freed
 * - Stack size cannot be altered
 *
 * @param[in] name human-readable thread name (can be NULL)
 * @param[in] stack_size stack size in bytes (cannot be changed later)
 * @param[in] callback pointer to a function to be executed in this thread
 * @param[in] context pointer to a user-specified object (will be passed to the callback)
 * @return pointer to the created RHSThread instance
 */
RHSThread* rhs_thread_alloc_service(
    const char* name,
    uint32_t stack_size,
    RHSThreadCallback callback,
    void* context);

void rhs_thread_free(RHSThread* thread);

/**
 * @brief Start a RHSThread instance.
 *
 * The thread MUST be stopped when calling this function.
 *
 * @param[in,out] thread pointer to the RHSThread instance to be started
 */
void rhs_thread_start(RHSThread* thread);

bool rhs_thread_join(RHSThread* thread);

RHSThreadId rhs_thread_get_current_id(void);

RHSThread* rhs_thread_get_current(void);

RHSThreadId rhs_thread_get_id(RHSThread* thread);

/**
 * @brief Set the thread flags of a RHSThread.
 *
 * Can be used as a simple inter-thread communication mechanism.
 *
 * @param[in] thread_id unique identifier of the thread to be notified
 * @param[in] flags bitmask of thread flags to set
 * @return bitmask combination of previous and newly set flags
 */
uint32_t rhs_thread_flags_set(RHSThreadId thread_id, uint32_t flags);

/**
 * @brief Clear the thread flags of the current RHSThread.
 *
 * @param[in] flags bitmask of thread flags to clear
 * @return bitmask of thread flags before clearing
 */
uint32_t rhs_thread_flags_clear(uint32_t flags);

/**
 * @brief Get the thread flags of the current RHSThread.
 * @return current bitmask of thread flags
 */
uint32_t rhs_thread_flags_get(void);

/**
 * @brief Wait for some thread flags to be set.
 *
 * @see RHSFlag for option and error flags.
 *
 * @param[in] flags bitmask of thread flags to wait for
 * @param[in] options combination of option flags determining the behavior of the function
 * @param[in] timeout maximum time to wait in milliseconds (use RHSWaitForever to wait forever)
 * @return bitmask combination of received thread and error flags
 */
uint32_t rhs_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout);

void rhs_thread_scrub(void);
