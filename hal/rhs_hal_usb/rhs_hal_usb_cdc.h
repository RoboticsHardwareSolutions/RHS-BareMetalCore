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

extern RHSHalUsbInterface usb_cdc_desc;

void rhs_hal_cdc_set_callbacks(uint8_t if_num, CdcCallbacks* cb, void* context);

cdc_line_coding_t* rhs_hal_cdc_get_port_settings(uint8_t if_num);

uint8_t rhs_hal_cdc_get_ctrl_line_state(uint8_t if_num);

void rhs_hal_cdc_send(uint8_t if_num, uint8_t* buf, uint16_t len);

int32_t rhs_hal_cdc_receive(uint8_t if_num, uint8_t* buf, uint16_t max_len);
