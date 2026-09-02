cmake_minimum_required(VERSION 3.24)
project(usb_eth_bridge C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC usb_eth_bridge.c)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Verify required targets are available
if(NOT TARGET tinyusb)
    message(FATAL_ERROR "usb_eth_bridge requires tinyusb")
endif()

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    mongoose
    PRIVATE
    rhs
    rhs_hal
)

start_up(usb_eth_bridge_start_up)
