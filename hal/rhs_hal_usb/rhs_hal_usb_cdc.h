#pragma once

#include <stdint.h>
#include "usb_cdc.h"

#if defined(STM32F103xE) /* There is not enough memory for double vcp */
#    define CDC_DATA_SZ 32
#else
#    define CDC_DATA_SZ 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void (*tx_ep_callback)(void* context);
    void (*rx_ep_callback)(void* context);
    void (*state_callback)(void* context, uint8_t state);
    void (*ctrl_line_callback)(void* context, uint8_t state);
    void (*config_callback)(void* context, struct usb_cdc_line_coding* config);
} CdcCallbacks;

void rhs_hal_cdc_set_callbacks(uint8_t if_num, CdcCallbacks* cb, void* context);

struct usb_cdc_line_coding* rhs_hal_cdc_get_port_settings(uint8_t if_num);

uint8_t rhs_hal_cdc_get_ctrl_line_state(uint8_t if_num);

void rhs_hal_cdc_send(uint8_t if_num, uint8_t* buf, uint16_t len);

int32_t rhs_hal_cdc_receive(uint8_t if_num, uint8_t* buf, uint16_t max_len);

#ifdef __cplusplus
}
#endif
