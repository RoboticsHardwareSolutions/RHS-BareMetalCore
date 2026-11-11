cmake_minimum_required(VERSION 3.24)
project(can_open C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
        can_open.c
        can_open_cli.c
        can_open_od.c
        can_open_pdo.c
        can_open_stack.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        rhs
        rhs_hal
        co_stack
        PRIVATE
        rtimer
        rcan
        cli
)

service(can_open_srv "can_open" 2048)
