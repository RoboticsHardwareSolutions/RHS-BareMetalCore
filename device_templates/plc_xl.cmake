set(RPLC_XL ON)

## set HAL
set(RHS_HAL_FLASH_EX ON)
set(RHS_HAL_IO ON)
set(RHS_HAL_RTC ON)
set(RHS_HAL_SERIAL ON)
set(RHS_HAL_SPEAKER ON)
set(RHS_HAL_CAN ON)
set(RHS_HAL_RANDOM ON)
set(RHS_HAL_NETWORK ON)

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
if(NOT RHS_TEST_LOG_SAVE_DISABLE)
    set(RHS_TEST_LOG_SAVE ON)
endif()

## set LAUNCHER
set(RHS_LAUNCH_GENERATOR_ENABLE ON)
set(DEVICE_TYPE "STM32F765ZG")
set(DEVICE_SVD_FILE "STM32F765.svd")


## compiler config
if(NOT LINKER_SCRIPT_NAME)
    set(LINKER_SCRIPT_NAME STM32F765ZGTX_FLASH.ld)
endif()

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT_NAME})

#Uncomment for hardware floating point
add_compile_definitions(ARM_MATH_CM7;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-Wl,-gc-sections,--print-memory-usage)
add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
#add_link_options(-T ${LINKER_SCRIPT})

#Uncomment for software floating point
#add_compile_options(-mfloat-abi=soft)

add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

# uncomment to mitigate c++17 absolute addresses warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

include_directories(
    core/inc
    core/inc/f7
    thirdparty/stm32f7_hal/Inc
    thirdparty/stm32f7_hal/Inc/Legacy
    thirdparty/stm32f7_cmsis/Include
    thirdparty/cmsis/CMSIS/Core/Include
)

add_definitions(-DUSE_HAL_DRIVER -DSTM32F765xx -DRPLC_XL)

file(GLOB_RECURSE SOURCES "core/src/syscalls.c" "core/src/sysmem.c" "core/src/f7/*.*" "thirdparty/stm32f7_hal/*.*" "thirdparty/stm32f7_cmsis/Source/Templates/gcc/startup_stm32f765xx.s")
list(FILTER SOURCES EXCLUDE REGEX "_template[.]c$")
