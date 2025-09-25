# RHS Application Registration System
# Optimized version for better performance and maintainability

set(RHS_OUTPUT_FILE "${CMAKE_BINARY_DIR}/applications.c")

# Header includes
set(RHS_INCLUDES "#include \"stdint.h\"\n#include \"applications.h\"\n\n")

# Initialize file once
file(WRITE "${RHS_OUTPUT_FILE}" "${RHS_INCLUDES}")

################################## SERVICES ##################################
# FreeRTOS Services - persistent tasks that should never terminate
set(RHS_SERVICE_EXTERN_SECTION "")
set(RHS_SERVICE_ARRAY_SECTION "")
set(RHS_SERVICE_BEGIN "const RHSInternalApplication RHS_SERVICES[] = {\n/* SERVICE_BEGIN */\n")
set(RHS_SERVICE_END "/* SERVICE_END */\n};\n")
set(RHS_SERVICE_COUNT "const short RHS_SERVICES_COUNT = (sizeof(RHS_SERVICES) / sizeof(RHS_SERVICES[0]));\n\n")

# Initialize services section
file(APPEND "${RHS_OUTPUT_FILE}" "${RHS_SERVICE_BEGIN}\n${RHS_SERVICE_END}\n${RHS_SERVICE_COUNT}")

################################## START UPs ##################################
# Initialization hooks - functions executed after all services are started
set(RHS_STARTUP_EXTERN_SECTION "")
set(RHS_STARTUP_ARRAY_SECTION "")
set(RHS_STARTUP_BEGIN "const RHSInternalOnStartHook RHS_START_UP[] = {\n/* START_UP_BEGIN */\n")
set(RHS_STARTUP_END "/* START_UP_END */\n};\n")
set(RHS_STARTUP_COUNT "const short RHS_START_UP_COUNT = (sizeof(RHS_START_UP) / sizeof(RHS_START_UP[0]));\n")

# Initialize startup section
file(APPEND "${RHS_OUTPUT_FILE}" "${RHS_STARTUP_BEGIN}\n${RHS_STARTUP_END}\n${RHS_STARTUP_COUNT}")

# Global lists to track registered services and startup hooks
set(RHS_REGISTERED_SERVICES "" CACHE INTERNAL "List of registered services")
set(RHS_REGISTERED_STARTUPS "" CACHE INTERNAL "List of registered startup hooks")

# Internal helper function to update file content efficiently
function(_rhs_update_file_content marker_begin marker_end new_content)
    file(READ "${RHS_OUTPUT_FILE}" FILE_CONTENT)
    string(REPLACE "${marker_end}" "${new_content}\n${marker_end}" FILE_CONTENT "${FILE_CONTENT}")
    file(WRITE "${RHS_OUTPUT_FILE}" "${FILE_CONTENT}")
endfunction()

# Register a FreeRTOS service (persistent task)
# Usage: service(handler_function "service_name" stack_size_bytes)
function(service service_handler service_name stack_size)
    # Input validation
    if(NOT service_handler)
        message(FATAL_ERROR "service(): service_handler is required")
    endif()
    if(NOT service_name)
        message(FATAL_ERROR "service(): service_name is required")
    endif()
    if(NOT stack_size)
        message(FATAL_ERROR "service(): stack_size is required")
    endif()
    
    # Validate stack size is numeric
    if(NOT stack_size MATCHES "^[0-9]+$")
        message(FATAL_ERROR "service(): stack_size must be a positive integer, got: ${stack_size}")
    endif()

    # Add extern declaration
    set(service_extern "extern int32_t ${service_handler}(void* context);\n")
    file(READ "${RHS_OUTPUT_FILE}" FILE_CONTENT)
    string(REPLACE "${RHS_SERVICE_BEGIN}" "${service_extern}${RHS_SERVICE_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")
    
    # Add service definition to array
    set(service_definition "    {\n        .app = ${service_handler},\n        .name = \"${service_name}\",\n        .stack_size = ${stack_size},\n    },\n")
    string(REPLACE "${RHS_SERVICE_END}" "${service_definition}${RHS_SERVICE_END}" FILE_CONTENT "${FILE_CONTENT}")
    
    file(WRITE "${RHS_OUTPUT_FILE}" "${FILE_CONTENT}")
    
    # Save service info to global list
    set(service_info "${service_name}:${service_handler}:${stack_size}")
    list(APPEND RHS_REGISTERED_SERVICES "${service_info}")
    set(RHS_REGISTERED_SERVICES "${RHS_REGISTERED_SERVICES}" CACHE INTERNAL "List of registered services")
    
    # Only link to rhs if it exists in this project
    if(TARGET rhs)
        target_link_libraries(rhs PUBLIC ${PROJECT_NAME})
    endif()
    
endfunction()

# Register a startup hook (initialization function)
# Usage: start_up(initialization_function)
function(start_up start_func)
    # Input validation
    if(NOT start_func)
        message(FATAL_ERROR "start_up(): start_func is required")
    endif()

    # Add extern declaration
    set(startup_extern "extern void ${start_func}(void);\n")
    file(READ "${RHS_OUTPUT_FILE}" FILE_CONTENT)
    string(REPLACE "${RHS_STARTUP_BEGIN}" "${startup_extern}${RHS_STARTUP_BEGIN}" FILE_CONTENT "${FILE_CONTENT}")
    
    # Add function to startup array
    set(startup_definition "    ${start_func},\n")
    string(REPLACE "${RHS_STARTUP_END}" "${startup_definition}${RHS_STARTUP_END}" FILE_CONTENT "${FILE_CONTENT}")
    
    file(WRITE "${RHS_OUTPUT_FILE}" "${FILE_CONTENT}")
    
    # Save startup hook info to global list
    list(APPEND RHS_REGISTERED_STARTUPS "${start_func}")
    set(RHS_REGISTERED_STARTUPS "${RHS_REGISTERED_STARTUPS}" CACHE INTERNAL "List of registered startup hooks")
    
    # Only link to rhs if it exists in this project
    if(TARGET rhs)
        target_link_libraries(rhs PUBLIC ${PROJECT_NAME})
    endif()
    
endfunction()

# Report function to display all registered services and startup hooks
function(report)
    message(STATUS "=== RHS Registration Report ===")
    
    # Report services
    list(LENGTH RHS_REGISTERED_SERVICES services_count)
    if(services_count GREATER 0)
        message(STATUS "Registered FreeRTOS Services (${services_count}):")
        foreach(service_info IN LISTS RHS_REGISTERED_SERVICES)
            string(REPLACE ":" ";" service_parts "${service_info}")
            list(GET service_parts 0 service_name)
            list(GET service_parts 1 service_handler)
            list(GET service_parts 2 stack_size)
            message(STATUS "  - ${service_name}: ${service_handler} (${stack_size} bytes)")
        endforeach()
    else()
        message(STATUS "No FreeRTOS services registered.")
    endif()
    
    message(STATUS "")
    
    # Report startup hooks
    list(LENGTH RHS_REGISTERED_STARTUPS startups_count)
    if(startups_count GREATER 0)
        message(STATUS "Registered Startup Hooks (${startups_count}):")
        foreach(startup_func IN LISTS RHS_REGISTERED_STARTUPS)
            message(STATUS "  - ${startup_func}")
        endforeach()
    else()
        message(STATUS "No startup hooks registered.")
    endif()
    
    message(STATUS "=== End Report ===")
endfunction()
