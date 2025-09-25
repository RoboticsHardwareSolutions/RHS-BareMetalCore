cmake_minimum_required(VERSION 3.24)
project(usb_cdc_net C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC usb_cdc_net.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Check if tinyusb and mongoose targets are available
if(NOT TARGET tinyusb OR NOT TARGET mongoose)
    message(FATAL_ERROR "tinyusb and mongoose libraries must be available to build usb_cdc_net")
endif()

# Enable RNDIS and ECM support in TinyUSB
target_compile_definitions(tinyusb PUBLIC -DCFG_TUD_ECM_RNDIS=1)

# Configure Mongoose options
target_compile_definitions(
        mongoose
        PUBLIC
        -DMG_ENABLE_TCPIP=1
        -DMG_ENABLE_CUSTOM_MILLIS=0
        -DMG_ENABLE_CUSTOM_RANDOM=1
        -DMG_ENABLE_PACKED_FS=1
        -DMG_IO_SIZE=512
        -DMG_ENABLE_LINES=1
)

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
        tinyusb
        mongoose
)

start_up(usb_cdc_net_start)
