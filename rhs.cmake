set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/applications.c")

set(INCLUDES "#include \"stdint.h\"\n#include \"applications.h\"\n")

file(WRITE "${OUTPUT_FILE}" "${INCLUDES}")

################################## SERVICES ##################################
set(SERVICE_BEGIN "const RHSInternalApplication RHS_SERVICES[] = {\n/* SERVICE_BEGIN */\n")
set(SERVICE_END "/* SERVICE_END */\n};\n")
set(SERVICE_COUNT "const short RHS_SERVICES_COUNT = (sizeof(RHS_SERVICES) / sizeof(RHS_SERVICES[0]));\n")

file(APPEND "${OUTPUT_FILE}" "${SERVICE_BEGIN}\n${SERVICE_END}\n${SERVICE_COUNT}")

function(service service_handler service_name stack_size)
    if (NOT service_handler OR NOT service_name OR NOT stack_size)
        message(FATAL_ERROR "add_freertos_service_definition: Missing required arguments.")
    endif ()

    file(READ "${OUTPUT_FILE}" FILE_CONTENT)

    set(service_definition "extern int32_t ${service_handler}(void* context);\n")
    string(REPLACE "${SERVICE_BEGIN}" "${service_definition}\n${SERVICE_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")

    set(service_definition_code
            "    {
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
    if (NOT start_func)
        message(FATAL_ERROR "Missing required arguments.")
    endif ()

    file(READ "${OUTPUT_FILE}" FILE_CONTENT)

    set(start_up_definition "extern void ${start_func}(void);\n")
    string(REPLACE "${START_UP_BEGIN}" "${start_up_definition}\n${START_UP_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")

    set(start_up_definition_code "${start_func},\n")
    string(REPLACE "${START_UP_END}" "${start_up_definition_code}\n${START_UP_END}" FILE_CONTENT "${FILE_CONTENT}")

    file(WRITE "${OUTPUT_FILE}" "${FILE_CONTENT}")
    target_link_libraries(rhs PUBLIC ${PROJECT_NAME})
endfunction()
