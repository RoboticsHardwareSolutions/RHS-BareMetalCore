cmake_minimum_required(VERSION 3.24)
project(usb_bridge C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
        usb_bridge.c
        usb_bridge_cli.c
        usb_bridge_can.c
        usb_bridge_serial.c
        usb_bridge_eth.c
        slcan.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
)

# Eth target: mongoose STM32F Ethernet driver
if(RHS_HAL_ETH)
        target_link_libraries(${PROJECT_NAME} PRIVATE mongoose)
endif()

start_up(usb_bridge_start_up)
