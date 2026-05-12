cmake_minimum_required(VERSION 3.24)
project(eth_net C)
set(CMAKE_C_STANDARD 11)

# eth_net configuration options
if(NOT DEFINED RHS_ETH_NET_IP)
    set(RHS_ETH_NET_IP "192.168.0.100" CACHE STRING "Static IP address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_GATEWAY)
    set(RHS_ETH_NET_GATEWAY "192.168.0.1" CACHE STRING "Gateway address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_NETMASK)
    set(RHS_ETH_NET_NETMASK "255.255.255.0" CACHE STRING "Network mask for eth_net service")
endif()

add_library(${PROJECT_NAME} STATIC
    eth_net.c
)

target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_IP_STRING="${RHS_ETH_NET_IP}")
target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_GW_STRING="${RHS_ETH_NET_GATEWAY}")
target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_MASK_STRING="${RHS_ETH_NET_NETMASK}")

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET mongoose)
    message(FATAL_ERROR "mongoose library not found, please add_subdirectory(mongoose) before eth_net")
endif()

# Configure Mongoose options for Ethernet networking
target_compile_definitions(
    mongoose
    PUBLIC
    MG_ENABLE_PACKED_FS=1
    MG_ENABLE_CUSTOM_MILLIS=0
    MG_ENABLE_MBEDTLS=0
    MG_ARCH=MG_ARCH_FREERTOS
    MG_ENABLE_CUSTOM_RANDOM=1
    MG_ENABLE_TCPIP=1
    MG_ENABLE_DRIVER_STM32F=1
    # MG_ENABLE_TCPIP_DRIVER_INIT if 1 (by default), call MG_TCPIP_DRIVER_INIT(&mgr) in mg_mgr_init().
    # If 0, call it manually later. Useful if you need several mgr, e.g. for dual interfaces (Eth + USB + WiFi)
    MG_ENABLE_TCPIP_DRIVER_INIT=0
    MG_TCPIP_PHY_ADDR=1
    MG_DRIVER_MDC_CR=4
    # If MG_ENABLE_PACKED_FS is enabled, disable MG_ENABLE_FILE
    MG_ENABLE_FILE=0
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
