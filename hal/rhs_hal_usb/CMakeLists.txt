cmake_minimum_required(VERSION 3.24)
project(rhs_hal_usb C)
set(CMAKE_C_STANDARD 11)

if(NOT TARGET usb_core)
        message(FATAL_ERROR "No usb_core")
endif()

add_library(${PROJECT_NAME} STATIC rhs_hal_usb.c rhs_hal_usb_cdc.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC rhs rhs_hal usb_core)
