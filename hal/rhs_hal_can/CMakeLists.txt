cmake_minimum_required(VERSION 3.24)
project(rhs_hal_can C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rhs_hal_can.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC rhs rhs_hal rcan)
