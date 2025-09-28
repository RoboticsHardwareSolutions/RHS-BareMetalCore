message("\tDRIVERS:")

if(RHS_DRIVER_EEPROM)
    add_subdirectory(eeprom)
    message("\t\tRHS_DRIVER_EEPROM\t- ON")
else()
    message("\t\tRHS_DRIVER_EEPROM\t- OFF")
endif()
