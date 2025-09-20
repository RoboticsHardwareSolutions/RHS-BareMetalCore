# CHANGELOG

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
