cmake_minimum_required(VERSION 3.24)
project(usb_eth_bridge C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC usb_eth_bridge.c)

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Verify required targets are available
if(NOT TARGET tinyusb OR NOT TARGET mongoose)
    message(FATAL_ERROR "usb_eth_bridge requires tinyusb and mongoose")
endif()

# Reuse ETH-net's Mongoose driver settings (stm32f driver, no full IP stack needed)
target_compile_definitions(
    mongoose
    PUBLIC
    MG_ENABLE_PACKED_FS=1
    MG_ENABLE_CUSTOM_MILLIS=0
    MG_ENABLE_MBEDTLS=0
    MG_ARCH=MG_ARCH_FREERTOS
    MG_ENABLE_CUSTOM_RANDOM=1
    MG_ENABLE_TCPIP=1
    MG_ENABLE_DRIVER_STM32F=1
    MG_ENABLE_TCPIP_DRIVER_INIT=0
    MG_TCPIP_PHY_ADDR=1
    MG_DRIVER_MDC_CR=4
    MG_ENABLE_FILE=0
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    mongoose
    eth_net   # for EthPhyConfig type and mg_tcpip_driver_stm32f_data
    PRIVATE
    rhs
    rhs_hal
    tinyusb
)
