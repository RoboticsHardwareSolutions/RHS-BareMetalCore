cmake_minimum_required(VERSION 3.24)
project(memmng_test C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC memmng_unit_test.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        rhs
        cli
)


start_up(test_rhs_memmgr_start_up)
