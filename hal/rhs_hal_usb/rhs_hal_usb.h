#pragma once

#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RHSHalUsbInterface RHSHalUsbInterface;

struct RHSHalUsbInterface {
    void (*init)(usbd_device* dev, RHSHalUsbInterface* intf, void* ctx);
    void (*deinit)(usbd_device* dev);
    void (*wakeup)(usbd_device* dev);
    void (*suspend)(usbd_device* dev);

    struct usb_device_descriptor* dev_descr;

    void* str_manuf_descr;
    void* str_prod_descr;
    void* str_serial_descr;

    void* cfg_descr;
};

/** USB device interface modes */
extern RHSHalUsbInterface usb_cdc_single;
extern RHSHalUsbInterface usb_cdc_dual;

typedef enum {
    RHSHalUsbStateEventReset,
    RHSHalUsbStateEventWakeup,
    RHSHalUsbStateEventSuspend,
    RHSHalUsbStateEventDescriptorRequest,
} RHSHalUsbStateEvent;

typedef void (*RHSHalUsbStateCallback)(RHSHalUsbStateEvent state, void* context);

/** USB device low-level initialization
 */
void rhs_hal_usb_init(void);

/** Set USB device configuration
 *
 * @param      mode new USB device mode
 * @param      ctx context passed to device mode init function
 * @return     true - mode switch started, false - mode switch is locked
 */
bool rhs_hal_usb_set_config(RHSHalUsbInterface* new_if, void* ctx);

/** Get USB device configuration
 *
 * @return    current USB device mode
 */
RHSHalUsbInterface* rhs_hal_usb_get_config(void);

/** Lock USB device mode switch
 */
void rhs_hal_usb_lock(void);

/** Unlock USB device mode switch
 */
void rhs_hal_usb_unlock(void);

/** Check if USB device mode switch locked
 * 
 * @return    lock state
 */
bool rhs_hal_usb_is_locked(void);

/** Disable USB device
 */
void rhs_hal_usb_disable(void);

/** Enable USB device
 */
void rhs_hal_usb_enable(void);

/** Set USB state callback
 */
void rhs_hal_usb_set_state_callback(RHSHalUsbStateCallback cb, void* ctx);

/** Restart USB device
 */
void rhs_hal_usb_reinit(void);

#ifdef __cplusplus
}
#endif
