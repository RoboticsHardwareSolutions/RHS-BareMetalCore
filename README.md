# Instruction

0. Just use [BMPLC_Quick_Project](https://github.com/RoboticsHardwareSolutions/BMPLC_Quick_Project.git) for BMPLCs

1. To include this module in your project:

```sh
$ git submodule add https://github.com/RoboticsHardwareSolutions/RHS-BareMetalCore.git thirdparty/rhs
```

2. In the top `CMakeLists.txt`, choose your BMPLC device and include `rhs.cmake`:
        
```cmake
set(BMPLC_XL ON) # or
set(BMPLC_L ON)  # or
set(BMPLC_M ON)

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
project(bmplc C CXX ASM)
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
# if you use just BMPLC that's enough
include_directories(core/inc) # include config files, for example FreeRTOSConfig.h

set(BMPLC_M ON) # Set BMPLC. If you do not use BMPLC, good luck! You have to include header files for HAL config

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

# Module index

## Core (`core/`)

| Module | Description | README |
|---|---|---|
| `thread` | FreeRTOS thread wrapper | [core/README.md](core/README.md) |
| `message_queue` | Thread-safe message queue | [core/README.md](core/README.md) |
| `event_flag` | Event flags (FreeRTOS event groups) | [core/README.md](core/README.md) |
| `mutex` | Recursive mutex wrapper | [core/README.md](core/README.md) |
| `semaphore` | Counting / binary semaphore | [core/README.md](core/README.md) |
| `timer` | Software timer wrapper | [core/README.md](core/README.md) |
| `stream_buf` | Stream buffer wrapper | [core/README.md](core/README.md) |
| `record` | Named object registry (publish/subscribe) | [core/README.md](core/README.md) |
| `api_lock` | Synchronous cross-thread API call helper | [core/README.md](core/README.md) |
| `log` | RTT-backed logging (`RHS_LOG_I/W/E`) | [core/README.md](core/README.md) |
| `check` | `rhs_assert` / `rhs_crash` with weak log hook | [core/README.md](core/README.md) |
| `memmgr` | Heap allocator wrappers | [core/README.md](core/README.md) |

## HAL (`hal/`)

| Module | CMake option / target | Description | README |
|---|---|---|---|
| `rhs_hal_can` | always on | CAN bus HAL | [hal/rhs_hal_can/README.md](hal/rhs_hal_can/README.md) |
| `rhs_hal_cortex` | always on | SysTick, NVIC helpers | [hal/rhs_hal_cortex/README.md](hal/rhs_hal_cortex/README.md) |
| `rhs_hal_flash_ex` | always on | External flash (mt25ql128aba) | [hal/rhs_hal_flash_ex/README.md](hal/rhs_hal_flash_ex/README.md) |
| `rhs_hal_gpio` | always on | GPIO abstraction | [hal/rhs_hal_gpio/README.md](hal/rhs_hal_gpio/README.md) |
| `rhs_hal_i2c` | always on | I2C HAL | [hal/rhs_hal_i2c/README.md](hal/rhs_hal_i2c/README.md) |
| `rhs_hal_interrupt` | always on | IRQ registration table | [hal/rhs_hal_interrupt/README.md](hal/rhs_hal_interrupt/README.md) |
| `rhs_hal_io` | always on | Digital I/O abstraction | [hal/rhs_hal_io/README.md](hal/rhs_hal_io/README.md) |
| `rhs_hal_power` | always on | Reset / power control | [hal/rhs_hal_power/README.md](hal/rhs_hal_power/README.md) |
| `rhs_hal_random` | always on | Hardware RNG | [hal/rhs_hal_random/README.md](hal/rhs_hal_random/README.md) |
| `rhs_hal_rtc` | always on | RTC + Unix time conversion | [hal/rhs_hal_rtc/README.md](hal/rhs_hal_rtc/README.md) |
| `rhs_hal_serial` | always on | UART HAL | [hal/rhs_hal_serial/README.md](hal/rhs_hal_serial/README.md) |
| `rhs_hal_speaker` | always on | PWM buzzer | [hal/rhs_hal_speaker/README.md](hal/rhs_hal_speaker/README.md) |
| `rhs_hal_usb` | always on | TinyUSB device stack + `tud_net_dispatch` | [hal/rhs_hal_usb/README.md](hal/rhs_hal_usb/README.md) |
| `rhs_hal_version` | always on | Device UID / version string | [hal/rhs_hal_version/README.md](hal/rhs_hal_version/README.md) |

## Drivers (`drivers/`)

| Module | CMake option / target | Description | README |
|---|---|---|---|
| `eeprom` | always on | I2C EEPROM read/write | [drivers/eeprom/README.md](drivers/eeprom/README.md) |

## Services (`applications/services/`)

| Module | CMake option | Description | README |
|---|---|---|---|
| `cli` | always on | Interactive command-line interface over RTT/UART | [applications/services/cli/README.md](applications/services/cli/README.md) |
| `loader` | always on | Dynamic application loader | [applications/services/loader/README.md](applications/services/loader/README.md) |
| `notification` | always on | LED / buzzer notification service | [applications/services/notification/README.md](applications/services/notification/README.md) |
| `can_open` | `RHS_APPLICATION_CAN_OPEN` | CANopen stack integration | [applications/services/can_open/README.md](applications/services/can_open/README.md) |
| `net` | `RHS_APPLICATION_ETH_NET` / `USB_CDC_NET` / `USB_ETH_BRIDGE` | Unified networking layer (Mongoose) | [applications/services/net/README.md](applications/services/net/README.md) |
| `net / eth_net` | `RHS_APPLICATION_ETH_NET` | Physical Ethernet via STM32F RMII MAC | [applications/services/net/eth_net/README.md](applications/services/net/eth_net/README.md) |
| `net / usb_cdc_net` | `RHS_APPLICATION_USB_CDC_NET` | USB device network (RNDIS/ECM/NCM) | [applications/services/net/usb_cdc_net/README.md](applications/services/net/usb_cdc_net/README.md) |
| `net / usb_eth_bridge` | `RHS_APPLICATION_USB_ETH_BRIDGE` | Layer-2 USB-to-Ethernet bridge | [applications/services/net/usb_eth_bridge/README.md](applications/services/net/usb_eth_bridge/README.md) |
| `net / modbus_tcp` | included with `net` | Modbus TCP server on top of a `Net` instance | [applications/services/net/README.md](applications/services/net/README.md) |
| `usb_serial_bridge` | `RHS_APPLICATION_USB_SERIAL_BRIDGE` | USB CDC serial bridge | [applications/services/usb_serial_bridge/README.md](applications/services/usb_serial_bridge/README.md) |

# RHS Core Library

## Custom Logging

The library provides a weak implementation of `rhs_log_save()` function that allows you to customize logging behavior for critical errors and assertions.

### How to implement custom logging

To define your own logging behavior, simply implement the `rhs_log_save` function in your project:

```c
#include <stdarg.h>
#include <stdio.h>

void rhs_log_save(char* str, ...) {
    va_list args;
    va_start(args, str);
    
    // Example: Print to console
    vprintf(str, args);
    
    // Example: Save to flash memory
    // flash_write_log(formatted_string);
    
    // Example: Send via UART
    // uart_send_log(formatted_string);
    
    va_end(args);
}
```

### Usage

The logging function is automatically called by:
- `rhs_assert()` - when assertion fails
- `rhs_crash()` - when system crash is triggered

If you don't implement `rhs_log_save()`, no logging will occur (weak function will be empty).
