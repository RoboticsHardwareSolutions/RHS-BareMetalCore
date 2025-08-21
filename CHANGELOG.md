# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Weak attribute for `rhs_log_save` function to allow custom logging implementation
- Enhanced error handling with custom logging capabilities

### Changed
- `rhs_log_save` function now uses weak linkage for user customization

## [0.0.1] - 2025-08-21

### Added
- Initial implementation of RHS core check functionality
- `rhs_assert` macro for runtime assertions
- `rhs_crash` macro for controlled system crashes
- `CallContext` structure for error context tracking

## usb bridge
- [x] Fix double mode (second bridge can't send data)
- [x] Two threads for two bridges
