cmake_minimum_required(VERSION 3.24)
project(usb_serial_bridge C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC usb_serial_bridge.c)

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

service(vcp_service "usb_serial_bridge" 4096)
