cmake_minimum_required(VERSION 3.24)
project(rhs_hal_gpio C)

# Header-only library – no compiled sources
add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(
        ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
