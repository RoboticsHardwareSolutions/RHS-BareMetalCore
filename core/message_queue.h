/**
 * @file message_queue.h
 * RHSMessageQueue
 */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHSMessageQueue RHSMessageQueue;

/** Allocate rhs message queue
 *
 * @param[in]  msg_count  The message count
 * @param[in]  msg_size   The message size
 *
 * @return     pointer to RHSMessageQueue instance
 */
RHSMessageQueue* rhs_message_queue_alloc(uint32_t msg_count, uint32_t msg_size);

/** Free queue
 *
 * @param      instance  pointer to RHSMessageQueue instance
 */
void rhs_message_queue_free(RHSMessageQueue* instance);

/** Put message into queue
 *
 * @param      instance  pointer to RHSMessageQueue instance
 * @param[in]  msg_ptr   The message pointer
 * @param[in]  timeout   The timeout
 *
 * @return     The rhs status.
 */
RHSStatus rhs_message_queue_put(RHSMessageQueue* instance, const void* msg_ptr, uint32_t timeout);

/** Get message from queue
 *
 * @param      instance  pointer to RHSMessageQueue instance
 * @param      msg_ptr   The message pointer
 * @param[in]  timeout   The timeout
 *
 * @return     The rhs status.
 */
RHSStatus rhs_message_queue_get(RHSMessageQueue* instance, void* msg_ptr, uint32_t timeout);

/** Get queue capacity
 *
 * @param      instance  pointer to RHSMessageQueue instance
 *
 * @return     capacity in object count
 */
uint32_t rhs_message_queue_get_capacity(RHSMessageQueue* instance);

/** Get message size
 *
 * @param      instance  pointer to RHSMessageQueue instance
 *
 * @return     Message size in bytes
 */
uint32_t rhs_message_queue_get_message_size(RHSMessageQueue* instance);

/** Get message count in queue
 *
 * @param      instance  pointer to RHSMessageQueue instance
 *
 * @return     Message count
 */
uint32_t rhs_message_queue_get_count(RHSMessageQueue* instance);

/** Get queue available space
 *
 * @param      instance  pointer to RHSMessageQueue instance
 *
 * @return     Message count
 */
uint32_t rhs_message_queue_get_space(RHSMessageQueue* instance);

/** Reset queue
 *
 * @param      instance  pointer to RHSMessageQueue instance
 *
 * @return     The rhs status.
 */
RHSStatus rhs_message_queue_reset(RHSMessageQueue* instance);

#ifdef __cplusplus
}
#endif
