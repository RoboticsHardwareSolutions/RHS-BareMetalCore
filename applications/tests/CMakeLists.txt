cmake_minimum_required(VERSION 3.24)
project(memmng_test C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME}
        STATIC
        memmng_unit_test.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
        cli
)

start_up(rhs_memmgr_test_start_up)

if(RPLC_XL)
        target_sources(${PROJECT_NAME} PRIVATE flash_ex_unit_test.c)
        start_up(flash_ex_test_start_up)
elseif(RPLC_L)
        target_sources(${PROJECT_NAME} PRIVATE flash_ex_unit_test.c)
        start_up(flash_ex_test_start_up)
elseif(RPLC_M)

else()

endif()
