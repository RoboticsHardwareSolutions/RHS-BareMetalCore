# Always-on services
add_subdirectory(cli)
add_subdirectory(loader)

# Optional services, see service_option() in cmake/rhs.cmake
message("\tSERVICES:")
service_option(RHS_SERVICE_NOTIFICATION notification)
service_option(RHS_SERVICE_CAN_OPEN can_open)
service_option(RHS_SERVICE_USB_SERIAL_BRIDGE usb_serial_bridge)

# Network stack, enabled by RHS_SERVICE_ETH_NET / RHS_SERVICE_USB_CDC_NET / RHS_SERVICE_USB_ETH_BRIDGE
add_subdirectory(net)
