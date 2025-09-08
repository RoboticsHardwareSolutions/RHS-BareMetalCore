cmake_minimum_required(VERSION 3.24)
project(usb_cdc_net C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC usb_cdc_net.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET tinyusb OR NOT TARGET mongoose)
    message(FATAL_ERROR "tinyusb and mongoose libraries must be available to build usb_cdc_net")
endif()

target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        rhs
        rhs_hal
        tinyusb
        mongoose
)

target_compile_definitions(tinyusb PUBLIC -DCFG_TUD_ECM_RNDIS=1)

service(cdc_net_service "cdc_net" 4096)
