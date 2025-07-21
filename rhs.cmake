set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/applications.c")

set(INCLUDES "#include \"stdint.h\"\n#include \"applications.h\"\n")

file(WRITE "${OUTPUT_FILE}" "${INCLUDES}")

################################## SERVICES ##################################
set(SERVICE_BEGIN "const RHSInternalApplication RHS_SERVICES[] = {\n/* SERVICE_BEGIN */\n")
set(SERVICE_END "/* SERVICE_END */\n};\n")
set(SERVICE_COUNT "const short RHS_SERVICES_COUNT = (sizeof(RHS_SERVICES) / sizeof(RHS_SERVICES[0]));\n")

file(APPEND "${OUTPUT_FILE}" "${SERVICE_BEGIN}\n${SERVICE_END}\n${SERVICE_COUNT}")

function(service service_handler service_name stack_size)
    if(NOT service_handler OR NOT service_name OR NOT stack_size)
        message(FATAL_ERROR "add_freertos_service_definition: Missing required arguments.")
    endif()

    file(READ "${OUTPUT_FILE}" FILE_CONTENT)

    set(service_definition "extern int32_t ${service_handler}(void* context);\n")
    string(REPLACE "${SERVICE_BEGIN}" "${service_definition}\n${SERVICE_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")

    set(service_definition_code
        "{
        .app = ${service_handler},
        .name = \"${service_name}\",
        .stack_size = ${stack_size},
    },\n")
    string(REPLACE "${SERVICE_END}" "${service_definition_code}\n${SERVICE_END}" FILE_CONTENT "${FILE_CONTENT}")

    file(WRITE "${OUTPUT_FILE}" "${FILE_CONTENT}")
    target_link_libraries(rhs PUBLIC ${PROJECT_NAME})
endfunction()

################################## START UPs ##################################
set(START_UP_BEGIN "const RHSInternalOnStartHook RHS_START_UP[] = {\n/* START_UP_BEGIN */\n")
set(START_UP_END "/* START_UP_END */\n};\n")
set(START_UP_COUNT "const short RHS_START_UP_COUNT = (sizeof(RHS_START_UP) / sizeof(RHS_START_UP[0]));\n")

file(APPEND "${OUTPUT_FILE}" "${START_UP_BEGIN}\n${START_UP_END}\n${START_UP_COUNT}")

function(start_up start_func)
    if(NOT start_func)
        message(FATAL_ERROR "Missing required arguments.")
    endif()

    file(READ "${OUTPUT_FILE}" FILE_CONTENT)

    set(start_up_definition "extern void ${start_func}(void);\n")
    string(REPLACE "${START_UP_BEGIN}" "${start_up_definition}\n${START_UP_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")

    set(start_up_definition_code "${start_func},\n")
    string(REPLACE "${START_UP_END}" "${start_up_definition_code}\n${START_UP_END}" FILE_CONTENT "${FILE_CONTENT}")

    file(WRITE "${OUTPUT_FILE}" "${FILE_CONTENT}")
    target_link_libraries(rhs PUBLIC ${PROJECT_NAME})
endfunction()


################################## CHECH RPLC ##################################

if(PLC_TYPE MATCHES "RPLC_XL")
    set(RPLC_XL ON)

    if(NOT LINKER_SCRIPT_NAME)
        set(LINKER_SCRIPT_NAME STM32F765ZGTX_FLASH.ld)
    endif()

    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT_NAME})

    #Uncomment for hardware floating point
    add_compile_definitions(ARM_MATH_CM7;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
    add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-Wl,-gc-sections,--print-memory-usage)
    add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
    #add_link_options(-T ${LINKER_SCRIPT})

    #Uncomment for software floating point
    #add_compile_options(-mfloat-abi=soft)

    add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
    add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

    # uncomment to mitigate c++17 absolute addresses warnings
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

    include_directories(
        core/inc/f7
        thirdparty/stm32f7_hal/Inc
        thirdparty/stm32f7_hal/Inc/Legacy
        thirdparty/stm32f7_cmsis/Include
        thirdparty/cmsis/CMSIS/Core/Include
    )

    add_definitions(-DUSE_HAL_DRIVER -DSTM32F765xx -DRPLC_XL)

    file(GLOB_RECURSE SOURCES "core/src/syscalls.c" "core/src/sysmem.c" "core/src/f7/*.*" "thirdparty/stm32f7_hal/*.*" "thirdparty/stm32f7_cmsis/Source/Templates/gcc/startup_stm32f765xx.s")
    list(FILTER SOURCES EXCLUDE REGEX "_template[.]c$")

elseif(PLC_TYPE MATCHES "RPLC_L")
    set(RPLC_L ON)

    if(NOT LINKER_SCRIPT_NAME)
        set(LINKER_SCRIPT_NAME STM32F765ZGTX_FLASH.ld)
    endif()

    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT_NAME})

    add_compile_definitions(ARM_MATH_CM7;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
    add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-Wl,-gc-sections,--print-memory-usage)
    add_link_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
    #add_link_options(-T ${LINKER_SCRIPT})

    #Uncomment for software floating point
    #add_compile_options(-mfloat-abi=soft)

    add_compile_options(-mcpu=cortex-m7 -mthumb -mthumb-interwork)
    add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

    # uncomment to mitigate c++17 absolute addresses warnings
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

    include_directories(
        core/inc/f7
        thirdparty/stm32f7_hal/Inc
        thirdparty/stm32f7_hal/Inc/Legacy
        thirdparty/stm32f7_cmsis/Include
        thirdparty/cmsis/CMSIS/Core/Include
    )

    add_definitions(-DUSE_HAL_DRIVER -DSTM32F765xx -DRPLC_L)

    file(GLOB_RECURSE SOURCES "core/src/syscalls.c" "core/src/sysmem.c" "core/src/f7/*.*" "thirdparty/stm32f7_hal/*.*" "thirdparty/stm32f7_cmsis/Source/Templates/gcc/startup_stm32f765xx.s")
    list(FILTER SOURCES EXCLUDE REGEX "_template[.]c$")

elseif(PLC_TYPE MATCHES "RPLC_M")
    set(RPLC_M ON)

    if(NOT LINKER_SCRIPT_NAME)
        set(LINKER_SCRIPT_NAME STM32F103RETX_FLASH.ld)
    endif()

    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT_NAME})

    add_compile_definitions(ARM_MATH_CM3;ARM_MATH_MATRIX_CHECK;ARM_MATH_ROUNDING)
    #add_compile_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    #add_link_options(-mfloat-abi=hard -mfpu=fpv4-sp-d16)
    add_link_options(-Wl,-gc-sections,--print-memory-usage)
    add_link_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
    #add_link_options(-T ${LINKER_SCRIPT})

    #Uncomment for software floating point
    #add_compile_options(-mfloat-abi=soft)

    add_compile_options(-mcpu=cortex-m3 -mthumb -mthumb-interwork)
    add_compile_options(-ffunction-sections -fdata-sections -fno-common -fmessage-length=0)

    # uncomment to mitigate c++17 absolute addresses warnings
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-register")

    include_directories(
        core/inc/f1
        thirdparty/stm32f1_hal/Inc
        thirdparty/stm32f1_hal/Inc/Legacy
        thirdparty/stm32f1_cmsis/Include
        thirdparty/cmsis/CMSIS/Core/Include
    )

    add_definitions(-DUSE_HAL_DRIVER -DSTM32F103xE -DRPLC_M)

    file(GLOB_RECURSE SOURCES "core/src/syscalls.c" "core/src/sysmem.c" "core/src/f1/*.*" "thirdparty/stm32f1_hal/*.*" "thirdparty/stm32f1_cmsis/Source/Templates/gcc/startup_stm32f103xe.s")
    list(FILTER SOURCES EXCLUDE REGEX "_template[.]c$")

else()

    message("You use custom BareMetal configuration. Good luck!")

endif()
