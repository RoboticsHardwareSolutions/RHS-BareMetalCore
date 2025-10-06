cmake_minimum_required(VERSION 3.24)
project(eth_net C)
set(CMAKE_C_STANDARD 11)

# eth_net service configuration options
# Can be overridden in device templates before including this library
if(NOT DEFINED RHS_ETH_NET_IP)
    set(RHS_ETH_NET_IP "192.168.1.100" CACHE STRING "Static IP address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_GATEWAY)
    set(RHS_ETH_NET_GATEWAY "192.168.1.1" CACHE STRING "Gateway address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_NETMASK)
    set(RHS_ETH_NET_NETMASK "255.255.255.0" CACHE STRING "Network mask for eth_net service")
endif()

# Macro to convert IP address from string format to Mongoose MG_IPV4 format
macro(rhs_ip_to_mongoose_format input_ip output_var)
    string(REPLACE "." ", " ${output_var} "${input_ip}")
    set(${output_var} "MG_IPV4(${${output_var}})")
endmacro()

# Convert RHS network parameters to Mongoose format
rhs_ip_to_mongoose_format("${RHS_ETH_NET_IP}" MG_TCPIP_IP_VALUE)
rhs_ip_to_mongoose_format("${RHS_ETH_NET_GATEWAY}" MG_TCPIP_GW_VALUE)
rhs_ip_to_mongoose_format("${RHS_ETH_NET_NETMASK}" MG_TCPIP_MASK_VALUE)

add_library(${PROJECT_NAME} STATIC 
    eth_net_srv.c
    eth_net_listeners.c
)

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
        # For static IP configuration, define MG_TCPIP_{IP,MASK,GW}
        # By default, those are set to zero, meaning that DHCP is used
        # Default network configuration can be overridden in device templates
        "MG_TCPIP_IP=${MG_TCPIP_IP_VALUE}"
        "MG_TCPIP_GW=${MG_TCPIP_GW_VALUE}"
        "MG_TCPIP_MASK=${MG_TCPIP_MASK_VALUE}"
        # Construct MAC address from the MCU unique ID. It is defined in the ST CMSIS header as UID_BASE
        "MGUID=((uint32_t*) 0x1FFF7A10)"  # Unique 96-bit chip ID
        # MG_SET_MAC_ADDRESS macro - complex macro moved to header file
        # If MG_ENABLE_PACKED_FS is enabled, disable MG_ENABLE_FILE
        MG_ENABLE_FILE=0
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        mongoose
        PRIVATE
        rhs
        rhs_hal
)

service(eth_net_service "eth_net" 16384)
