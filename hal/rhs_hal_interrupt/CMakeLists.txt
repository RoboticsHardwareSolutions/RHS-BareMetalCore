cmake_minimum_required(VERSION 3.24)
project(rhs_hal_interrupt C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rhs_hal_interrupt.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC rhs)

if(TARGET tinyusb)
        target_link_libraries(${PROJECT_NAME} PUBLIC tinyusb)
        target_compile_definitions(${PROJECT_NAME} PUBLIC TINYUSB)
endif()
