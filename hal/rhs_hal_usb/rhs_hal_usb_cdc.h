#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"

typedef enum
{
    CdcStateDisconnected,
    CdcStateConnected,
} CdcState;

typedef enum
{
    CdcCtrlLineDTR = (1 << 0),
    CdcCtrlLineRTS = (1 << 1),
} CdcCtrlLine;

typedef struct
{
    void (*tx_callback)(void* context);
    void (*rx_callback)(void* context);
    void (*state_callback)(void* context, CdcState state);
    void (*ctrl_line_callback)(void* context, CdcCtrlLine ctrl_lines);
    void (*line_config_callback)(void* context, cdc_line_coding_t const* config);
} CdcCallbacks;

/** USB interface with one CDC port, set by rhs_hal_cdc_acquire() */
extern RHSHalUsbInterface usb_cdc_desc;

/** Reserve the CDC port for one user (usb_bridge) and bring USB up as a CDC device.
 * Callbacks are set separately with rhs_hal_cdc_set_callbacks().
 * @return false when the port is already reserved */
bool rhs_hal_cdc_acquire(void);

/** Clear the callbacks, turn the CDC USB interface off and free the port */
void rhs_hal_cdc_release(void);

void rhs_hal_cdc_set_callbacks(CdcCallbacks* cb, void* context);

cdc_line_coding_t* rhs_hal_cdc_get_port_settings(void);

uint8_t rhs_hal_cdc_get_ctrl_line_state(void);

/** @return Number of bytes accepted by the TX FIFO: less than @p len when it is full */
uint16_t rhs_hal_cdc_send(const uint8_t* buf, uint16_t len);

int32_t rhs_hal_cdc_receive(uint8_t* buf, uint16_t max_len);
