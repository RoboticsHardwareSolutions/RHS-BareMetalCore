# The usb_bridge eth target needs the mongoose configuration below too
if(NOT (RHS_SERVICE_ETH_NET OR RHS_SERVICE_USB_CDC_NET OR (RHS_SERVICE_USB_BRIDGE AND RHS_HAL_ETH)))
        message("\t\tRHS_SERVICE_ETH_NET / USB_CDC_NET  - OFF")
        return()
endif()

cmake_minimum_required(VERSION 3.24)
project(net C)
set(CMAKE_C_STANDARD 11)

if(NOT TARGET mongoose)
        message(FATAL_ERROR "mongoose library not found, please add_subdirectory(mongoose)")
endif()

add_library(${PROJECT_NAME} STATIC
        net.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        net_utils
        mongoose
        rhs
        rhs_hal
        PRIVATE
        net_listeners
)

# Mongoose configuration shared by all network backends
target_compile_definitions(
        mongoose
        PUBLIC
        MG_ENABLE_PACKED_FS=1
        MG_ENABLE_CUSTOM_MILLIS=0
        MG_ENABLE_MBEDTLS=0
        MG_ENABLE_FREERTOS=1
        MG_ENABLE_TCPIP=1
        MG_ENABLE_DRIVER_STM32F=1
        MG_ENABLE_TCPIP_DRIVER_INIT=0
        MG_TCPIP_PHY_ADDR=1
        MG_DRIVER_MDC_CR=4
        MG_ENABLE_FILE=0
        MG_ARCH=MG_ARCH_ARMGCC
        MG_ENABLE_CUSTOM_LOG=1
)

# Mongoose log output goes to RHS log (MG_ENABLE_CUSTOM_LOG)
target_sources(mongoose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/net_mg_log.c)
target_include_directories(
        mongoose
        PUBLIC
        $<BUILD_INTERFACE:${rhs_SOURCE_DIR}>
)

service_option(RHS_SERVICE_USB_CDC_NET usb_cdc_net)
service_option(RHS_SERVICE_ETH_NET eth_net)

add_subdirectory(net_listeners)
add_subdirectory(net_utils)
add_subdirectory(modbus_tcp)
