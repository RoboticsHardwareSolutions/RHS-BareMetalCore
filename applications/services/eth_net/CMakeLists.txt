cmake_minimum_required(VERSION 3.24)
project(eth_net C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC eth_net_srv.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET mongoose)
        message(FATAL_ERROR "mongoose library not found, please add_subdirectory(mongoose) before eth_net")
endif()


target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        mongoose
        PRIVATE
        rhs
        rhs_hal
        rpc
)

service(eth_net_service "eth_net" 16384)
