#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "tusb.h"

typedef struct RHSHalUsbInterface RHSHalUsbInterface;

struct RHSHalUsbInterface
{
    void (*init)(void* context);
    void (*deinit)(void* context);
    void (*wakeup)(void);
    void (*suspend)(void);

    void* context;

    tusb_desc_device_t const* device_desc;
    uint8_t const* const*     configuration_arr;
    char const* const*        string_desc_arr;
    size_t                    string_desc_arr_count;
};

/** USB device low-level initialization
 */
void rhs_hal_usb_init(void);

void rhs_hal_usb_disable(void);

/** @return false when rhs_hal_usb_init() left USB off (BMPLC_XL with strap PG10 low: PA11/PA12 belong to CAN1) */
bool rhs_hal_usb_is_available(void);

/** Restart USB device
 */
void rhs_hal_usb_reinit(void);

void rhs_hal_usb_set_interface(RHSHalUsbInterface* iface, void* context);

RHSHalUsbInterface* rhs_hal_usb_get_interface(void);

/** Mount callback
 *
 * mounted: the host configured the device and the bus is not suspended. There is no VBUS sensing, so a pulled cable
 * shows up as suspend (mounted false); the next mount comes with the enumeration after the cable is back.
 * Stopping the USB interface (rhs_hal_usb_set_interface) also reports false.
 *
 * @warning    Called in the TinyUSB task thread (or in the thread stopping the interface), on state changes only:
 *             do not block, hand the work over to your own thread.
 */
typedef void (*RHSHalUsbMountCallback)(bool mounted, void* context);

#define RHS_HAL_USB_MOUNT_CALLBACKS_MAX 4

/** Subscribe to mount state changes, the current state is rhs_hal_usb_is_mounted()
 * @return false when RHS_HAL_USB_MOUNT_CALLBACKS_MAX callbacks are already set */
bool rhs_hal_usb_add_mount_callback(RHSHalUsbMountCallback callback, void* context);

/** Unsubscribe the pair given to rhs_hal_usb_add_mount_callback(), no-op when it is not set */
void rhs_hal_usb_remove_mount_callback(RHSHalUsbMountCallback callback, void* context);

/** @return true when the host configured the device and the bus is not suspended */
bool rhs_hal_usb_is_mounted(void);

#ifdef __cplusplus
}
#endif
