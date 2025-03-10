cmake_minimum_required(VERSION 3.24)
project(rhs_hal_serial C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rhs_hal_serial.c internal/hal_rs232.c internal/hal_rs485.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
)

target_link_libraries(
        ${PROJECT_NAME} PRIVATE rhs rhs_hal rserial)
