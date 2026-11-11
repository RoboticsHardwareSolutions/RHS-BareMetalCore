# Always-on services
add_subdirectory(cli)
add_subdirectory(loader)

# Optional services, see service_option() in cmake/rhs.cmake
message("\tSERVICES:")
service_option(RHS_SERVICE_NOTIFICATION notification)
service_option(RHS_SERVICE_USB_BRIDGE usb_bridge)

# Network stack, enabled by RHS_SERVICE_ETH_NET / RHS_SERVICE_USB_CDC_NET (mongoose setup also for the usb_bridge eth target)
add_subdirectory(net)
