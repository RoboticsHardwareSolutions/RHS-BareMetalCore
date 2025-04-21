#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct UsbSerialBridge UsbSerialBridge;

typedef struct {
    uint8_t vcp_ch;
    uint8_t serial_ch;
    uint8_t flow_pins;
    uint8_t baudrate_mode;
    uint32_t baudrate;
    uint8_t software_de_re;
} UsbSerialConfig;

typedef struct {
    uint32_t rx_cnt;
    uint32_t tx_cnt;
    uint32_t baudrate_cur;
} UsbSerialState;

UsbSerialBridge* usb_serial_enable(UsbSerialConfig* cfg);

void usb_serial_disable(UsbSerialBridge* usb_serial);

void usb_serial_set_config(UsbSerialBridge* usb_serial, UsbSerialConfig* cfg);

void usb_serial_get_config(UsbSerialBridge* usb_serial, UsbSerialConfig* cfg);

void usb_serial_get_state(UsbSerialBridge* usb_serial, UsbSerialState* st);
