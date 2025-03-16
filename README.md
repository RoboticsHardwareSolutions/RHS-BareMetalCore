# Instruction

0. Just use [RPLC_Quick_Project](https://github.com/RoboticsHardwareSolutions/RPLC_Quick_Project.git) for RPLCs

1. To include this module in your project:

```sh
$ git submodule add https://github.com/RoboticsHardwareSolutions/RHS-BareMetalCore.git thirdparty/rhs
```

2. In the top `CMakeLists.txt`, choose your RPLC device and include `rhs.cmake`:
        
```cmake
set(RPLC_XL ON) # or
set(RPLC_L ON)  # or
set(RPLC_M ON)

include(thirdparty/rhs/rhs.cmake)
```


3. Clone the following submodules to `thirdparty`:
- [CMSIS](https://github.com/ARM-software/CMSIS_5)
- STM32_CMSIS ([STM32F1_CMSIS for example](https://github.com/STMicroelectronics/cmsis-device-f1))
- STM32_HAL ([STM32F1_HAL for example](https://github.com/STMicroelectronics/stm32f1xx-hal-driver))
- [freertos-kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel.git)
- [RTT](https://github.com/SEGGERMicro/RTT.git)

4. In `thirdparty/CMakeLists.txt`:
```cmake
####################### FREERTOS lib ############################
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
                        INTERFACE
                        ../Core/Inc
)

target_compile_definitions(freertos_config
                        INTERFACE
                        projCOVERAGE_TEST=0
)

add_subdirectory(freertos)

####################### RTT lib ############################
add_library(RTT STATIC
                        RTT/RTT/SEGGER_RTT_ASM_ARMv7M.S
                        RTT/RTT/SEGGER_RTT_printf.c
                        RTT/RTT/SEGGER_RTT.c
)

target_compile_definitions(RTT PUBLIC -DBUFFER_SIZE_DOWN=64)

target_include_directories(RTT
                        PUBLIC
                        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/RTT/RTT>
                        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/RTT/Config>
)

####################### RHS lib ############################
add_subdirectory(rhs)
```

5. In the top `CMakeLists.txt`:

```cmake
###################### COMMON SETTINGS #########################
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
cmake_minimum_required(VERSION 3.24)

# specify cross compilers and tools
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# project settings
project(rplc C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
                message(STATUS "Maximum optimization for speed")
                add_compile_options(-Ofast)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
                message(STATUS "Maximum optimization for speed, debug info included")
                add_compile_options(-Ofast -g)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
                message(STATUS "Maximum optimization for size")
                add_compile_options(-Os)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinOptDebInfo")
                message(STATUS "Maximum optimization for size")
                add_compile_options(-Og -g)
else()
                message(STATUS "No optimization")
endif()

###################### LIBS SETTINGS #########################
# if you use just RPLC that's enough
include_directories(core/inc) # include config files, for example FreeRTOSConfig.h

set(RPLC_M ON) # Set RPLC. If you do not use RPLC, good luck! You have to include header files for HAL config

include(thirdparty/rhs/rhs.cmake) # include Definitions and functions for RHS

###################### YOUR PROPERTIES FOR TARGETS (IT'S EXAMPLE) #########################

# Create target
add_executable(${PROJECT_NAME}.elf
        main.c
        ${SOURCES} # It's in the rhs.cmake, don't worry
        ${LINKER_SCRIPT}
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME}.elf PUBLIC rhs rhs_hal)
target_link_options(${PROJECT_NAME}.elf PUBLIC -Wl,-gc-sections,--print-memory-usage,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map)
target_link_options(${PROJECT_NAME}.elf PUBLIC -T ${LINKER_SCRIPT})

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")

add_subdirectory(thirdparty)
add_subdirectory(rlibs)
```


# TODO

- [ ] Add generation for applications
- [services](applications/services/README.md)
- [tests](applications/tests/README.md)
- [core](core/README.md)
- [hal](hal/README.md)
