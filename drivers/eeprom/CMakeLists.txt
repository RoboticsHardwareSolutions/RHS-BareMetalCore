cmake_minimum_required(VERSION 3.10)

# EEPROM HAL Library
add_library(eeprom
    eeprom.c
)

target_include_directories(eeprom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(eeprom 
    rhs
    rhs_hal
)