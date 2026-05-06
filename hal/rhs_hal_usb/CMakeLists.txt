cmake_minimum_required(VERSION 3.24)
project(rhs_hal_usb C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC rhs_hal_usb.c rhs_hal_usb_cdc.c)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        rhs
        rhs_hal
)

if(NOT TARGET tinyusb)
        message(FATAL_ERROR "tinyusb target is not found. Please add tinyusb as a submodule (https://github.com/hathach/tinyusb.git) to thyrdparty directory.
In CMakeLists.txt of thirdparty directory add:

add_library(tinyusb STATIC
        tinyusb/src/tusb.c
        tinyusb/src/common/tusb_fifo.c
        tinyusb/src/device/usbd.c
        tinyusb/src/device/usbd_control.c
        tinyusb/src/class/net/ecm_rndis_device.c
        tinyusb/src/class/net/ncm_device.c
        tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c # For stm32f1 etc. which has no USB FS peripheral
        tinyusb/src/portable/st/stm32_fsdev/fsdev_common.c # For stm32f1 etc. which has no USB FS peripheral
        tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c # For stm32f4 etc. which has USB FS/HS peripheral with DWC IP
        tinyusb/src/portable/synopsys/dwc2/dwc2_common.c # For stm32f4 etc. which has USB FS/HS peripheral with DWC IP
        tinyusb/lib/networking/rndis_reports.c

        tinyusb/src/class/cdc/cdc_device.c
        rhs/hal/rhs_hal_usb/usb_descriptors.c
)

target_include_directories(tinyusb
        PUBLIC
        \$\{CMAKE_CURRENT_SOURCE_DIR\}/tinyusb/src
        \$\{CMAKE_CURRENT_SOURCE_DIR\}/tinyusb/src/portable/synopsys/dwc2 # For stm32f4 etc. which has USB FS/HS peripheral with DWC IP
        \$\{CMAKE_CURRENT_SOURCE_DIR\}/tinyusb/lib/networking
)

target_compile_options(tinyusb PRIVATE -Wno-conversion -Wno-sign-conversion)
target_link_libraries(tinyusb PRIVATE freertos_kernel)"
        )
endif()
