cmake_minimum_required(VERSION 3.24)
project(modbus_tcp)

add_library(${PROJECT_NAME} modbus_tcp.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        eth_net
        PRIVATE
        rhs
)
