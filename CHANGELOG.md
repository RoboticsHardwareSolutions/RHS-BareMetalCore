# CHANGELOG

## [Latest] - 2025-09-08

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
- [x] Fix double mode (second bridge can't send data)
- [x] Two threads for two bridges
