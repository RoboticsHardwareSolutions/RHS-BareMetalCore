cmake_minimum_required(VERSION 3.24)
project(net_listeners)

add_library(${PROJECT_NAME} net_listeners.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        mongoose
        PRIVATE
        rhs
)
