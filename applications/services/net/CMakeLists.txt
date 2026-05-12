cmake_minimum_required(VERSION 3.24)
project(net)

add_library(${PROJECT_NAME} net.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        net_utils
        mongoose
        PRIVATE
        rhs
        rhs_hal
        net_listeners
)

if(RHS_APPLICATION_USB_CDC_NET)
    message("\t\tRHS_APPLICATION_USB_CDC_NET\t\t- ON")
    add_subdirectory(usb_cdc_net)
    message("\t\t\tIP: ${RHS_CDC_NET_IP}")
    message("\t\t\tGateway: ${RHS_CDC_NET_GATEWAY}")
    message("\t\t\tNetmask: ${RHS_CDC_NET_NETMASK}")
else()
    message("\t\tRHS_APPLICATION_USB_CDC_NET\t\t- OFF")
endif()
if(RHS_SERVICE_ETH_NET)
    message("\t\tRHS_SERVICE_ETH_NET\t\t\t\t- ON")
    add_subdirectory(eth_net)
    message("\t\t\tIP: ${RHS_ETH_NET_IP}")
    message("\t\t\tGateway: ${RHS_ETH_NET_GATEWAY}")
    message("\t\t\tNetmask: ${RHS_ETH_NET_NETMASK}")
else()
    message("\t\tRHS_SERVICE_ETH_NET\t\t\t\t- OFF")
endif()

if(RHS_SERVICE_ETH_NET OR RHS_APPLICATION_USB_CDC_NET)
    add_subdirectory(net_listeners)
    add_subdirectory(net_utils)
    add_subdirectory(modbus_tcp)
endif()
