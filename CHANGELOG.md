# CHANGELOG

## [Unreleased]
### Added
- New `usb_bridge` service (`RHS_SERVICE_USB_BRIDGE`): one USB bridge with targets CAN (SLCAN over CDC), RS232, RS485 (raw bytes over CDC) and Eth (L2 frames over NCM, boards with `RHS_HAL_ETH`), and modes Bridge (USB <-> device), App (USB <-> application API) and Bridge+App; one bridge at a time; CLI `usb <target> [bridge|app|both] [baud]`, `usb stop`, `usb state`
- `rhs_hal_can` on STM32G0B1 (FDCAN1 PB8/PB9 through rcan), in `rhs_hal_can_fdcan.c`; interrupt `RHSHalInterruptIdFDCANIT0`
- `rhs_hal_cdc_acquire()` / `rhs_hal_cdc_release()`: reserve the USB CDC port
- `rhs_hal_usb_is_available()`: false on BMPLC_XL with strap PG10 low
- `usb_bridge_check()`: why a bridge cannot start; the CLI `usb` prints it, e.g. `error: eth bridge: not supported on this board: no Ethernet (RHS_HAL_ETH)` on G0B1 or `error: can bridge: ... CAN and USB share SRAM on STM32F103 ...` on BMPLC_M (CAN target there only in app mode)

### Changed
- **Breaking:** `usb_serial_bridge` and `net/usb_eth_bridge` replaced by `usb_bridge` (targets RS232/RS485 and Eth): `usb_serial_bridge_*` / `UsbSerialBridgeConfig` / `usb_eth_bridge_*` removed, CLI `usb_bridge -rs232|-rs485` and `usb_eth_bridge_app` replaced by `usb rs232|rs485|eth`; `RHS_SERVICE_USB_SERIAL_BRIDGE` / `RHS_SERVICE_USB_ETH_BRIDGE` map to `RHS_SERVICE_USB_BRIDGE` with a deprecation warning. Unimplemented `flow_pins` / `baudrate_mode` / `software_de_re` dropped
- **Breaking:** USB CDC has one port: `CFG_TUD_CDC 1`, dual CDC descriptor removed, `rhs_hal_cdc_*` functions lose the port number argument, `usb_single_cdc_desc` renamed to `usb_cdc_desc`; the CLI stays on RTT
- `rhs_hal_serial` is no longer enabled by a service, only by `RHS_HAL_SERIAL`
- Services follow one layout and naming scheme, see `applications/services/README.md`: `<name>.h` / `<name>_i.h` / `<name>.c`, entry points `<name>_srv` / `<name>_start_up`, `TAG "<name>"`, one CMakeLists template
- CMake options `RHS_APPLICATION_ETH_NET` / `USB_CDC_NET` / `USB_ETH_BRIDGE` renamed to `RHS_SERVICE_*`, `RHS_CDC_NET_*` renamed to `RHS_USB_CDC_NET_*` (old names still accepted with a deprecation warning)
- New `service_option()` CMake helper for optional services
- **Breaking:** `usb_serial_enable/disable/set_config/get_config/get_state` renamed to `usb_serial_bridge_start/stop/set_config/get_config/get_state`, `UsbSerialConfig/State` to `UsbSerialBridgeConfig/State`, `EthPhyConfig` to `EthNetPhyConfig`, `od_common_defs.h` to `can_open_od.h`, CMake targets `cdc_net` and `can_open_srv` to `usb_cdc_net` and `can_open`
- `net` no longer depends on the rhs checkout path (`CMAKE_SOURCE_DIR/thirdparty/rhs`)

### Fixed
- USB CDC stop hung on STM32F7: the USB clock was disabled before the TinyUSB task ran `tusb_deinit()`, the dwc2 core reset then waited forever
- `can_open`: events arriving together (e.g. RX and TX) were dropped

### Documentation
- README for `cli`, `loader`, `notification`, `can_open`, `usb_bridge` and for the services layout

## [0.0.6] - 2026-06-21
### Added
- New `usb_eth_bridge` service: transparent Layer-2 bridge between USB CDC-Net and physical Ethernet MAC — no IP stack on the device
- New `rhs_hal_cdc_net` module in `rhs_hal_usb`: virtual dispatch table that routes TinyUSB network callbacks (`tud_network_recv_cb`, `tud_network_xmit_cb`, `tud_network_init_cb`) to the currently active USB network backend at runtime, allowing `usb_cdc_net` and `usb_eth_bridge` to coexist in the same binary and be switched dynamically
- MS OS 2.0 BOS descriptor in `usb_cdc_net` for automatic NCM driver binding on Windows 10+

### Documentation
- Added README for `applications/services/net` (module tree, API summary, quick examples)
- Added README for `eth_net`, `usb_cdc_net`, `usb_eth_bridge`
- Added module index table to root README

## [0.0.5] - 2025-09-27
### Added
- New RHS_HAL_I2C support with configuration and initialization
- EEPROM driver with read, write, and erase functionalities with I2C communication
- New module with drivers and RHS_DRIVER_EEPROM configuration option for device templates

## [0.0.4] - 2025-09-20

### Added
- New ethernet network service (`eth_net`) with HTTP and TCP listener support

### Removed
- LWIP network stack dependency 

## [0.0.3] - 2025-09-09

### Added
- Records test suite with comprehensive test coverage
- RHS_TEST_RECORDS configuration option for device templates
- New records_test module for testing record creation, existence checking, and cleanup

## [0.0.2] - 2025-09-08

### Added
- USB CDC and RNDIS support with dual interfaces
- New `usb_cdc_net` service implementation
- Dynamic USB descriptors support
- Global TUSB configuration with string descriptor indices

### Changed
- Refactored USB service architecture: moved from `usb_net` to `usb_cdc_net`
- Updated CMake configuration for enhanced HAL configuration checks
- Improved USB bridge implementation with tinyUSB driver

### Removed
- Old `hal_usb` implementation
- Unused RndisMessage struct and related code
- Legacy USB bridge code

### Fixed
- USB CDC stop hung on STM32F7: the USB clock was disabled before the TinyUSB task ran `tusb_deinit()`, the dwc2 core reset then waited forever
- USB reinitialization issues
- USB bridge restoration functionality

## Previous Changes

### usb bridge

All notable changes to this project will be documented in this file.

## [0.0.1] - 2025-08-21

### Added
- Initial implementation of RHS core check functionality
- `rhs_assert` macro for runtime assertions
- `rhs_crash` macro for controlled system crashes
- `CallContext` structure for error context tracking
- Weak attribute for `rhs_log_save` function to allow custom logging implementation
- Enhanced error handling with custom logging capabilities

### Changed
- `rhs_log_save` function now uses weak linkage for user customization
