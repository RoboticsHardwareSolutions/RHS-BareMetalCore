cmake_minimum_required(VERSION 3.24)
project(stm32f_eth_driver C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC stm32f_eth_driver.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET mongoose)
        message(FATAL_ERROR "mongoose library not found, please add_subdirectory(mongoose) before stm32f_eth_driver")
endif()

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        mongoose
        PRIVATE
        rhs
        rhs_hal
)
