cmake_minimum_required(VERSION 3.24)
project(can_open_srv C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC can_open_srv.c can_open_srv_i.c can_open_ctrl_dev.c od_common_defs.c can_open_cli.c)

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

service(can_open_service "can_open" 2048)
