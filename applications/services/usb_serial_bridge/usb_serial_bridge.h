#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct UsbSerialBridge UsbSerialBridge;

typedef struct
{
    uint8_t  vcp_ch;
    uint8_t  serial_ch;
    uint8_t  flow_pins;
    uint8_t  baudrate_mode;
    uint32_t baudrate;
    uint8_t  software_de_re;
} UsbSerialBridgeConfig;

typedef struct
{
    uint32_t rx_cnt;
    uint32_t tx_cnt;
    uint32_t baudrate_cur;
} UsbSerialBridgeState;

UsbSerialBridge* usb_serial_bridge_start(UsbSerialBridgeConfig* cfg);

void usb_serial_bridge_stop(UsbSerialBridge* usb_serial);

void usb_serial_bridge_set_config(UsbSerialBridge* usb_serial, UsbSerialBridgeConfig* cfg);

void usb_serial_bridge_get_config(UsbSerialBridge* usb_serial, UsbSerialBridgeConfig* cfg);

void usb_serial_bridge_get_state(UsbSerialBridge* usb_serial, UsbSerialBridgeState* st);
