cmake_minimum_required(VERSION 3.24)
project(net_utils)

add_library(${PROJECT_NAME} net_utils.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

