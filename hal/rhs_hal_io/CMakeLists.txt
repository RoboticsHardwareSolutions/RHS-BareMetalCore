cmake_minimum_required(VERSION 3.24)
project(rhs_hal_io C)

add_library(${PROJECT_NAME} STATIC rhs_hal_io.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
