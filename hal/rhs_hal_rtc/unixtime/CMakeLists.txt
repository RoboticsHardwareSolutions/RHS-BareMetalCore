cmake_minimum_required(VERSION 3.24)
project(unixtime C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC unixtime.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
