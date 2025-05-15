cmake_minimum_required(VERSION 3.24)
project(rhs C)
set(CMAKE_C_STANDARD 11)

message("\n\n")
message("|--------------------------------------------------|")
message("|  Configuration ROBOTICS HARDWARE SOLUTIONS core  |")
message("|--------------------------------------------------|")

add_library(${PROJECT_NAME} STATIC
        rhs.c
        core/mutex.c
        core/memmgr.c
        core/kernel.c
        core/message_queue.c
        core/timer.c
        core/event_flag.c
        core/check.c
        core/log.c
        core/thread.c
        core/thread_list.c
        core/stream_buf.c
        core/semaphore.c
)
target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/applications.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET freertos_kernel)
        message(FATAL_ERROR
                "freertos_kernel target is not found. Please add freertos_kernel as a submodule (https://github.com/FreeRTOS/FreeRTOS-Kernel.git) to thyrdparty directory.
In CMakeLists.txt of thyrdparty directory add:
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
        INTERFACE
        ../Core/Inc
)
target_compile_definitions(freertos_config
        INTERFACE
        projCOVERAGE_TEST=0
)
set(FREERTOS_HEAP \"4\" CACHE STRING \"\" FORCE)
set(FREERTOS_PORT \"GCC_ARM_CM7\" CACHE STRING \"\" FORCE)
add_subdirectory(freertos)"
        )
endif()

if(NOT TARGET RTT)
        message(FATAL_ERROR
                "RTT target is not found. Please add RTT as a submodule (https://github.com/SEGGERMicro/RTT.git) to thyrdparty directory.
In CMakeLists.txt of thyrdparty directory add:
add_library(RTT STATIC
        RTT/RTT/SEGGER_RTT_ASM_ARMv7M.S
        RTT/RTT/SEGGER_RTT_printf.c
        RTT/RTT/SEGGER_RTT.c
)

target_compile_definitions(RTT PUBLIC -DBUFFER_SIZE_DOWN=64)

target_include_directories(RTT
        PUBLIC
        $<BUILD_INTERFACE:\${CMAKE_CURRENT_SOURCE_DIR}/RTT/RTT>
        $<BUILD_INTERFACE:\${CMAKE_CURRENT_SOURCE_DIR}/RTT/Config>
)"
        )
endif()

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        freertos_kernel
        RTT
        PRIVATE
        rhs_hal_cortex
        rhs_hal_power
)

if(RPLC_XL)
        message("\nRPLC_XL:")
        target_compile_definitions(${PROJECT_NAME} PUBLIC -DRPLC_XL)

        ## set HAL
        set(RHS_HAL_FLASH_EX ON)
        set(RHS_HAL_IO ON)
        set(RHS_HAL_RTC ON)
        set(RHS_HAL_SERIAL ON)
        set(RHS_HAL_SPEAKER ON)
        set(RHS_HAL_CAN ON)
        set(RHS_HAL_RANDOM ON)
        # set(RHS_HAL_NETWORK ON)

        ## set SERVICES
        if(NOT RHS_SERVICE_NOTIFICATION_DISABLE)
                set(RHS_SERVICE_NOTIFICATION ON)
        endif()
        if(NOT RHS_SERVICE_LWIP_DISABLE)
                set(RHS_SERVICE_LWIP ON)
        endif()
        if(NOT RHS_CAN_OPEN_DISABLE)
                set(RHS_CAN_OPEN ON)
        endif()
        ## set TESTS
        if(NOT RHS_TEST_MEMMNG_DISABLE)
                set(RHS_TEST_MEMMNG ON)
        endif()
        if(NOT RHS_TESTFLASH_EX_DISABLE)
                set(RHS_TESTFLASH_EX ON)
        endif()
elseif(RPLC_L)
        message("\n\nRPLC_L:")
        target_compile_definitions(${PROJECT_NAME} PUBLIC -DRPLC_L)

        ## set HAL
        set(RHS_HAL_FLASH_EX ON)
        set(RHS_HAL_IO ON)
        set(RHS_HAL_RTC ON)
        set(RHS_HAL_SERIAL ON)
        set(RHS_HAL_SPEAKER ON)
        set(RHS_HAL_CAN ON)
        set(RHS_HAL_RANDOM ON)

        ## set SERVICES
        if(NOT RHS_SERVICE_NOTIFICATION_DISABLE)
                set(RHS_SERVICE_NOTIFICATION ON)
        endif()
        if(NOT RHS_CAN_OPEN_DISABLE)
                set(RHS_CAN_OPEN ON)
        endif()
        ## set TESTS
        if(NOT RHS_TEST_MEMMNG_DISABLE)
                set(RHS_TEST_MEMMNG ON)
        endif()
        if(NOT RHS_TESTFLASH_EX_DISABLE)
                set(RHS_TESTFLASH_EX ON)
        endif()
elseif(RPLC_M)
        message("\n\nRPLC_M:")
        target_compile_definitions(${PROJECT_NAME} PUBLIC -DRPLC_M)

        ## set HAL
        set(RHS_HAL_SERIAL ON)
        set(RHS_HAL_SPEAKER ON)
        set(RHS_HAL_CAN ON)
        set(RHS_HAL_USB ON)

        ## set SERVICES
        if(NOT RHS_SERVICE_NOTIFICATION_DISABLE)
                set(RHS_SERVICE_NOTIFICATION ON)
        endif()
        if(NOT RHS_SERVICE_USB_SERIAL_BRIDGE_DISABLE)
                set(RHS_SERVICE_USB_SERIAL_BRIDGE ON)
        endif()
        if(NOT RHS_SERVICE_CAN_OPEN_DISABLE)
                set(RHS_SERVICE_CAN_OPEN ON)
        endif()
        ## set TESTS
        if(NOT RHS_TEST_MEMMNG_DISABLE)
                set(RHS_TEST_MEMMNG ON)
        endif()
else()
        message("\n\nCustom:")
endif()

add_subdirectory(hal)
add_subdirectory(applications)

message("\tLAUNCHER:")

if(NOT RHS_LAUNCH_IN_DISABLE)
        message("\t\tGENERATOR - ON")
        if(RPLC_XL OR RPLC_L OR RPLC_M)
                if(RPLC_XL OR RPLC_L)
                        set(DEVICE_TYPE "STM32F765ZG")
                        set(DEVICE_SVD_FILE "STM32F765.svd")
                elseif(RPLC_M)
                        set(DEVICE_TYPE "STM32F103RE")
                        set(DEVICE_SVD_FILE "STM32F103.svd")
                endif()
                set(LAUNCH_HEADER "/* FILE GENERATED BY CMAKE. SHOULD NOT BE MODIFIED */")

                configure_file(
                        "${CMAKE_CURRENT_SOURCE_DIR}/launch.json.in"
                        "${CMAKE_SOURCE_DIR}/.vscode/launch.json"
                        @ONLY
                )
        endif()
else()
        message("\t\tGENERATOR - OFF")
endif()
message("\n\n")
