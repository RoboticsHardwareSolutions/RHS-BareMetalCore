cmake_minimum_required(VERSION 3.24)
project(rhs_hal_flash_ex C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rhs_hal_flash_ex.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        mt25ql128aba
)


add_subdirectory(mt25ql128aba)
