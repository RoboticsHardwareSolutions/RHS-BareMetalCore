# usb_serial_bridge

Bridges a USB CDC virtual COM port to an RS232 / RS485 serial port.

- CMake option: `RHS_SERVICE_USB_SERIAL_BRIDGE` (also enables `rhs_hal_serial`, `rhs_hal_usb`, tinyusb)
- Start-up hook: `usb_serial_bridge_start_up` (registers the CLI command, no own task)
- CLI: `usb_bridge`

## API (`usb_serial_bridge.h`)

| Function | Description |
|---|---|
| `usb_serial_bridge_start(cfg)` | Start a bridge, returns a handle |
| `usb_serial_bridge_stop(bridge)` | Stop the bridge and free it |
| `usb_serial_bridge_set_config(bridge, cfg)` | Apply a new `UsbSerialBridgeConfig` |
| `usb_serial_bridge_get_config(bridge, cfg)` | Read the current config |
| `usb_serial_bridge_get_state(bridge, state)` | Read RX/TX counters and baudrate (`UsbSerialBridgeState`) |

```c
UsbSerialBridgeConfig cfg = {
    .vcp_ch    = 0,
    .serial_ch = RHSHalSerialIdRS485,
    .baudrate  = 9600,
};
UsbSerialBridge* bridge = usb_serial_bridge_start(&cfg);
```

## CLI

`usb_bridge -rs232` / `usb_bridge -rs485` toggles the bridge for that port (9600 baud, the next free VCP channel).
