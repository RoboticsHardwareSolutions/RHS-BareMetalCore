#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** USB device low-level initialization
 */
void rhs_hal_usb_init(void);

/** Restart USB device
 */
void rhs_hal_usb_reinit(void);

#ifdef __cplusplus
}
#endif
