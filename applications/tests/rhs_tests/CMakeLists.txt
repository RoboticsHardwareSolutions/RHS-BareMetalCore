cmake_minimum_required(VERSION 3.24)
project(rhs_test C)
set(CMAKE_C_STANDARD 11)

set(TEST_SOURCES)

message("\tTESTS:")
if(RHS_TEST_MEMMNG)
        message("\t\tRHS_TEST_MEMMNG\t\t- ON")
        list(APPEND TEST_SOURCES memmng_unit_test.c)
        start_up(rhs_memmgr_test_start_up)
else()
        message("\t\tRHS_TEST_MEMMNG\t\t- OFF")
endif()
if(RHS_TEST_RECORDS)
        message("\t\tRHS_TEST_RECORDS\t\t- ON")
        add_subdirectory(records_test)
else()
        message("\t\tRHS_TEST_MEMMNG\t\t- OFF")
endif()
if(RHS_TESTFLASH_EX)
        message("\t\tRHS_TESTFLASH_EX\t- ON")
        list(APPEND TEST_SOURCES flash_ex_unit_test.c)
        start_up(flash_ex_test_start_up)
else()
        message("\t\tRHS_TESTFLASH_EX\t- OFF")
endif()

if(TEST_SOURCES)
        add_library(${PROJECT_NAME}
                STATIC
                ${TEST_SOURCES}
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
                runit
        )
endif()
