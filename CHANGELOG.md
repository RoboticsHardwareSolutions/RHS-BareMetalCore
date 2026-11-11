# CHANGELOG

## [Unreleased]
### Changed
- Services follow one layout and naming scheme, see `applications/services/README.md`: `<name>.h` / `<name>_i.h` / `<name>.c`, entry points `<name>_srv` / `<name>_start_up`, `TAG "<name>"`, one CMakeLists template
- CMake options `RHS_APPLICATION_ETH_NET` / `USB_CDC_NET` / `USB_ETH_BRIDGE` renamed to `RHS_SERVICE_*`, `RHS_CDC_NET_*` renamed to `RHS_USB_CDC_NET_*` (old names still accepted with a deprecation warning)
- New `service_option()` CMake helper for optional services
- **Breaking:** `usb_serial_enable/disable/set_config/get_config/get_state` renamed to `usb_serial_bridge_start/stop/set_config/get_config/get_state`, `UsbSerialConfig/State` to `UsbSerialBridgeConfig/State`, `EthPhyConfig` to `EthNetPhyConfig`, `od_common_defs.h` to `can_open_od.h`, CMake targets `cdc_net` and `can_open_srv` to `usb_cdc_net` and `can_open`
- `net` no longer depends on the rhs checkout path (`CMAKE_SOURCE_DIR/thirdparty/rhs`)

### Fixed
- `can_open`: events arriving together (e.g. RX and TX) were dropped

### Documentation
- README for `cli`, `loader`, `notification`, `can_open`, `usb_serial_bridge` and for the services layout

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
