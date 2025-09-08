#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "tusb.h"

typedef struct RHSHalUsbInterface RHSHalUsbInterface;

struct RHSHalUsbInterface
{
    void (*init)(RHSHalUsbInterface* intf, void* ctx);
    void (*deinit)(void);
    void (*wakeup)(void);
    void (*suspend)(void);

    tusb_desc_device_t const* device_desc;
    uint8_t const* const*     configuration_arr;
    char const* const*        string_desc_arr;
};

/** USB device low-level initialization
 */
void rhs_hal_usb_init(void);

/** Restart USB device
 */
void rhs_hal_usb_reinit(void);

void rhs_hal_usb_set_interface(RHSHalUsbInterface* iface);

#ifdef __cplusplus
}
#endif
