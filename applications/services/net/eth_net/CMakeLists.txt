cmake_minimum_required(VERSION 3.24)
project(eth_net C)
set(CMAKE_C_STANDARD 11)

# eth_net configuration options
if(NOT DEFINED RHS_ETH_NET_IP)
    set(RHS_ETH_NET_IP "192.168.0.100" CACHE STRING "Static IP address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_GATEWAY)
    set(RHS_ETH_NET_GATEWAY "192.168.0.1" CACHE STRING "Gateway address for eth_net service")
endif()

if(NOT DEFINED RHS_ETH_NET_NETMASK)
    set(RHS_ETH_NET_NETMASK "255.255.255.0" CACHE STRING "Network mask for eth_net service")
endif()

add_library(${PROJECT_NAME} STATIC
    eth_net.c
)

target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_IP_STRING="${RHS_ETH_NET_IP}")
target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_GW_STRING="${RHS_ETH_NET_GATEWAY}")
target_compile_definitions(${PROJECT_NAME} PUBLIC ETH_NET_MASK_STRING="${RHS_ETH_NET_NETMASK}")

target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    mongoose
    net
    PRIVATE
    rhs
    rhs_hal
)
