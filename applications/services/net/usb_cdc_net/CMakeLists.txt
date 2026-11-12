cmake_minimum_required(VERSION 3.24)
project(usb_cdc_net C)
set(CMAKE_C_STANDARD 11)

if(NOT TARGET tinyusb)
        message(FATAL_ERROR "usb_cdc_net requires tinyusb")
endif()

set(RHS_USB_CDC_NET_IP "192.168.0.100" CACHE STRING "Static IP address for usb_cdc_net service")
set(RHS_USB_CDC_NET_GATEWAY "192.168.0.1" CACHE STRING "Gateway address for usb_cdc_net service")
set(RHS_USB_CDC_NET_NETMASK "255.255.255.0" CACHE STRING "Network mask for usb_cdc_net service")
message("\t\t\tIP: ${RHS_USB_CDC_NET_IP}")
message("\t\t\tGateway: ${RHS_USB_CDC_NET_GATEWAY}")
message("\t\t\tNetmask: ${RHS_USB_CDC_NET_NETMASK}")

add_library(${PROJECT_NAME} STATIC
        usb_cdc_net.c
)

target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC
        USB_CDC_NET_IP_STRING="${RHS_USB_CDC_NET_IP}"
        USB_CDC_NET_GW_STRING="${RHS_USB_CDC_NET_GATEWAY}"
        USB_CDC_NET_MASK_STRING="${RHS_USB_CDC_NET_NETMASK}"
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_definitions(
        mongoose
        PUBLIC
        MG_IO_SIZE=512
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        mongoose
        net
        PRIVATE
        rhs
        rhs_hal
)
