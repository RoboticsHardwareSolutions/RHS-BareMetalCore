cmake_minimum_required(VERSION 3.24)
project(rndis C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rndis.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
        tinyusb
        mongoose
)

service(rndis_service "rndis" 4096)
