/**
 * @file event_flag.h
 * RHS Event Flag
 */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHSEventFlag RHSEventFlag;

/** Allocate RHSEventFlag
 *
 * @return     pointer to RHSEventFlag
 */
RHSEventFlag* rhs_event_flag_alloc(void);

/** Deallocate RHSEventFlag
 *
 * @param      instance  pointer to RHSEventFlag
 */
void rhs_event_flag_free(RHSEventFlag* instance);

/** Set flags
 *
 * @warning    result of this function can be flags that you've just asked to
 *             set or not if someone was waiting for them and asked to clear it.
 *             It is highly recommended to read this function and
 *             xEventGroupSetBits source code.
 *
 * @param      instance  pointer to RHSEventFlag
 * @param[in]  flags     The flags to set
 *
 * @return     Resulting flags(see warning) or error (RHSStatus)
 */
uint32_t rhs_event_flag_set(RHSEventFlag* instance, uint32_t flags);

/** Clear flags
 *
 * @param      instance  pointer to RHSEventFlag
 * @param[in]  flags     The flags
 *
 * @return     Resulting flags or error (RHSStatus)
 */
uint32_t rhs_event_flag_clear(RHSEventFlag* instance, uint32_t flags);

/** Get flags
 *
 * @param      instance  pointer to RHSEventFlag
 *
 * @return     Resulting flags
 */
uint32_t rhs_event_flag_get(RHSEventFlag* instance);

/** Wait flags
 *
 * @param      instance  pointer to RHSEventFlag
 * @param[in]  flags     The flags
 * @param[in]  options   The option flags
 * @param[in]  timeout   The timeout
 *
 * @return     Resulting flags or error (RHSStatus)
 */
uint32_t rhs_event_flag_wait(RHSEventFlag* instance, uint32_t flags, uint32_t options, uint32_t timeout);

#ifdef __cplusplus
}
#endif
