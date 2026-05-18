#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Virtual dispatch table for TinyUSB network callbacks.
 *
 * Register an instance with tud_net_dispatch_set() to route the three
 * TinyUSB network callbacks to the currently active USB network backend
 * (usb_cdc_net or usb_eth_bridge).  Only one backend may be active at a time.
 */
typedef struct
{
    bool (*recv)(const uint8_t* buf, uint16_t len);
    void (*init)(void);
    uint16_t (*xmit)(uint8_t* dst, void* ref, uint16_t arg);
} TudNetOps;

/**
 * @brief Set the active TinyUSB network backend.
 *
 * Must be called before tusb_init() / after the previous backend is stopped.
 * @param ops  Pointer to a statically allocated ops table, or NULL to clear.
 */
void tud_net_dispatch_set(const TudNetOps* ops);

/**
 * @brief Clear the active backend (equivalent to tud_net_dispatch_set(NULL)).
 */
void tud_net_dispatch_clear(void);
