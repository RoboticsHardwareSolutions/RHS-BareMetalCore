cmake_minimum_required(VERSION 3.24)
project(cdc_net C)
set(CMAKE_C_STANDARD 11)

# cdc_net configuration options
if(NOT DEFINED RHS_CDC_NET_IP)
    set(RHS_CDC_NET_IP "192.168.3.100" CACHE STRING "Static IP address for cdc_net service")
endif()

if(NOT DEFINED RHS_CDC_NET_GATEWAY)
    set(RHS_CDC_NET_GATEWAY "192.168.3.1" CACHE STRING "Gateway address for cdc_net service")
endif()

if(NOT DEFINED RHS_CDC_NET_NETMASK)
    set(RHS_CDC_NET_NETMASK "255.255.255.0" CACHE STRING "Network mask for cdc_net service")
endif()

add_library(${PROJECT_NAME} STATIC usb_cdc_net.c)

target_compile_definitions(${PROJECT_NAME} PUBLIC CDC_NET_IP_STRING="${RHS_CDC_NET_IP}")
target_compile_definitions(${PROJECT_NAME} PUBLIC CDC_NET_GW_STRING="${RHS_CDC_NET_GATEWAY}")
target_compile_definitions(${PROJECT_NAME} PUBLIC CDC_NET_MASK_STRING="${RHS_CDC_NET_NETMASK}")

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Check if tinyusb and mongoose targets are available
if(NOT TARGET tinyusb OR NOT TARGET mongoose)
        message(FATAL_ERROR "tinyusb and mongoose libraries must be available to build usb_cdc_net")
endif()

# Configure Mongoose options
target_compile_definitions(
        mongoose
        PUBLIC
        -DMG_ENABLE_TCPIP=1
        -DMG_ENABLE_CUSTOM_MILLIS=0
        -DMG_ENABLE_CUSTOM_RANDOM=1
        -DMG_ENABLE_PACKED_FS=1
        -DMG_IO_SIZE=512
        # -DMG_ENABLE_LINES=1
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
