cmake_minimum_required(VERSION 3.24)
project(io C)

add_library(${PROJECT_NAME} STATIC io.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME} PUBLIC
        rhs
        rhs_hal_gpio
)

if(TARGET rhs_hal)
    target_link_libraries(rhs_hal PUBLIC ${PROJECT_NAME})
    target_compile_definitions(rhs_hal PUBLIC -DRHS_BSP_IO)
endif()
