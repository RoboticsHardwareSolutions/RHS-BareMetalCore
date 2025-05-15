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
