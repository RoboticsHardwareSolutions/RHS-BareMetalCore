cmake_minimum_required(VERSION 3.24)
project(lwip_service C)
set(CMAKE_C_STANDARD 11)

add_library(${PROJECT_NAME} STATIC
        App/lwip.c
        Target/ethernetif.c
        Target/arch/sys_arch.c
        Target/CMSIS_RTOS_V2/cmsis_os2.c
        lan8742/lan8742.c
)

target_include_directories(
        ${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/App
        ${CMAKE_CURRENT_SOURCE_DIR}/Target
        ${CMAKE_CURRENT_SOURCE_DIR}/Target/CMSIS_RTOS_V2
        ${CMAKE_CURRENT_SOURCE_DIR}/lan8742
)

set(STATIC_IP "192.168.1.100" CACHE STRING "Custom static IP")
set(STATIC_MASK "255.255.255.0" CACHE STRING "Custom mask")
set(STATIC_GATEWAY "192.168.1.1" CACHE STRING "Custom gateway")

function(validate_ip ip_var)
        if(NOT ${${ip_var}} MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
                message(FATAL_ERROR "Invalid IP format in ${ip_var}: '${${ip_var}}'. Expected: 'X.X.X.X' where X is 0-255")
        endif()

        # Проверка каждого октета (0-255)
        foreach(octet ${CMAKE_MATCH_1} ${CMAKE_MATCH_2} ${CMAKE_MATCH_3} ${CMAKE_MATCH_4})
                if(octet GREATER 255)
                        message(FATAL_ERROR "Invalid octet in ${ip_var}: '${octet}'. Must be ≤ 255")
                endif()
        endforeach()
endfunction()

validate_ip(STATIC_IP)
validate_ip(STATIC_MASK)
validate_ip(STATIC_GATEWAY)

add_compile_definitions(STATIC_IP_ADDRESS="${STATIC_IP}")
add_compile_definitions(STATIC_MASK_ADDRESS="${STATIC_MASK}")
add_compile_definitions(STATIC_GATEWAY_ADDRESS="${STATIC_GATEWAY}")

if(NOT TARGET lwip)
        message(FATAL_ERROR
                "
        set(LWIP_DIR \${CMAKE_CURRENT_SOURCE_DIR}/lwip)
        include(\${LWIP_DIR}/src/Filelists.cmake)
        add_library(lwip \${lwipnoapps_SRCS})
        target_include_directories(lwip PUBLIC \${LWIP_DIR}/src/include)
        target_link_libraries(lwip PUBLIC freertos_kernel lwip_service)"
        )
endif()


target_link_libraries(${PROJECT_NAME} PUBLIC rhs freertos_kernel lwip)

service(ethernet_service "ethernet" 1024)
