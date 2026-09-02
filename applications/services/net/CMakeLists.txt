if(RHS_APPLICATION_ETH_NET OR RHS_APPLICATION_USB_CDC_NET OR RHS_APPLICATION_USB_ETH_BRIDGE)
    cmake_minimum_required(VERSION 3.24)
    project(net)

    add_library(${PROJECT_NAME} net.c)

    target_include_directories(
        ${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    if(NOT TARGET mongoose)
        message(FATAL_ERROR "mongoose library not found, please add_subdirectory(mongoose)")
    endif()

    target_compile_definitions(
        mongoose
        PUBLIC
        MG_ENABLE_PACKED_FS=1
        MG_ENABLE_CUSTOM_MILLIS=0
        MG_ENABLE_MBEDTLS=0
        MG_ENABLE_FREERTOS=1
        MG_ENABLE_TCPIP=1
        MG_ENABLE_DRIVER_STM32F=1
        MG_ENABLE_TCPIP_DRIVER_INIT=0
        MG_TCPIP_PHY_ADDR=1
        MG_DRIVER_MDC_CR=4
        MG_ENABLE_FILE=0
        MG_ARCH=MG_ARCH_ARMGCC
    )

    target_link_libraries(
        ${PROJECT_NAME}
        PUBLIC
        net_utils
        mongoose
        rhs
        rhs_hal
        PRIVATE
        net_listeners
    )

    if(RHS_APPLICATION_USB_CDC_NET)
        message("\t\tRHS_APPLICATION_USB_CDC_NET\t\t- ON")
        add_subdirectory(usb_cdc_net)
        message("\t\t\tIP: ${RHS_CDC_NET_IP}")
        message("\t\t\tGateway: ${RHS_CDC_NET_GATEWAY}")
        message("\t\t\tNetmask: ${RHS_CDC_NET_NETMASK}")
    else()
        message("\t\tRHS_APPLICATION_USB_CDC_NET\t\t- OFF")
    endif()
    if(RHS_APPLICATION_ETH_NET)
        message("\t\tRHS_APPLICATION_ETH_NET\t\t\t\t- ON")
        add_subdirectory(eth_net)
        message("\t\t\tIP: ${RHS_ETH_NET_IP}")
        message("\t\t\tGateway: ${RHS_ETH_NET_GATEWAY}")
        message("\t\t\tNetmask: ${RHS_ETH_NET_NETMASK}")
    else()
        message("\t\tRHS_APPLICATION_ETH_NET\t\t\t\t- OFF")
    endif()

    if(RHS_APPLICATION_USB_ETH_BRIDGE)
        message("\t\tRHS_APPLICATION_USB_ETH_BRIDGE\t\t- ON")
        add_subdirectory(usb_eth_bridge)
    else()
        message("\t\tRHS_APPLICATION_USB_ETH_BRIDGE\t\t- OFF")
    endif()

    add_subdirectory(net_listeners)
    add_subdirectory(net_utils)
    add_subdirectory(modbus_tcp)
endif()
