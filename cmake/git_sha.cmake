find_program(GIT_EXECUTABLE git)

# Function to get version info from a git repository/submodule
function(get_git_version_info WORKING_DIR PREFIX)
    if (GIT_EXECUTABLE)
        # Get git SHA
        execute_process(COMMAND
                "${GIT_EXECUTABLE}" rev-parse --short HEAD
                WORKING_DIRECTORY "${WORKING_DIR}"
                OUTPUT_VARIABLE ${PREFIX}_GIT_SHA1
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

        # Try multiple strategies to get version tag
        set(GIT_TAG "")
        
        # Strategy 1: Get tag from current HEAD (works in detached HEAD state)
        execute_process(COMMAND git describe --tags --exact-match HEAD
                WORKING_DIRECTORY "${WORKING_DIR}"
                RESULT_VARIABLE GIT_TAG_RESULT
                OUTPUT_VARIABLE GIT_TAG
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        
        # Strategy 2: Get nearest tag (works for commits after tags)
        if(NOT GIT_TAG_RESULT EQUAL 0 OR NOT GIT_TAG)
            execute_process(COMMAND git describe --tags --abbrev=0
                    WORKING_DIRECTORY "${WORKING_DIR}"
                    RESULT_VARIABLE GIT_TAG_RESULT
                    OUTPUT_VARIABLE GIT_TAG
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
        
        # Strategy 3: Get any tag pointing to HEAD
        if(NOT GIT_TAG_RESULT EQUAL 0 OR NOT GIT_TAG)
            execute_process(COMMAND git tag --points-at HEAD
                    WORKING_DIRECTORY "${WORKING_DIR}"
                    OUTPUT_VARIABLE GIT_TAGS_LIST
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(GIT_TAGS_LIST)
                string(REGEX REPLACE "\n.*" "" GIT_TAG "${GIT_TAGS_LIST}")
                set(GIT_TAG_RESULT 0)
            endif()
        endif()

        if (GIT_TAG_RESULT EQUAL 0 AND GIT_TAG)
            # Remove leading 'v' or 'V'
            string(REGEX REPLACE "^[vV](.*)$" "\\1" ${PREFIX}_VERSION "${GIT_TAG}")
            
            # Try to extract version components
            # Match patterns like: 1.2.3, 10.4.3, 7.54, 0.18.0, etc.
            if("${${PREFIX}_VERSION}" MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
                set(${PREFIX}_VERSION_MAJOR "${CMAKE_MATCH_1}")
                set(${PREFIX}_VERSION_MINOR "${CMAKE_MATCH_2}")
                set(${PREFIX}_VERSION_PATCH "${CMAKE_MATCH_3}")
            elseif("${${PREFIX}_VERSION}" MATCHES "^([0-9]+)\\.([0-9]+)")
                set(${PREFIX}_VERSION_MAJOR "${CMAKE_MATCH_1}")
                set(${PREFIX}_VERSION_MINOR "${CMAKE_MATCH_2}")
                set(${PREFIX}_VERSION_PATCH "0")
            elseif("${${PREFIX}_VERSION}" MATCHES "^([0-9]+)")
                set(${PREFIX}_VERSION_MAJOR "${CMAKE_MATCH_1}")
                set(${PREFIX}_VERSION_MINOR "0")
                set(${PREFIX}_VERSION_PATCH "0")
            else()
                # Fallback for non-standard version format
                set(${PREFIX}_VERSION_MAJOR "0")
                set(${PREFIX}_VERSION_MINOR "0")
                set(${PREFIX}_VERSION_PATCH "0")
            endif()
            
            message(STATUS "${PREFIX} Version (from git tag): ${${PREFIX}_VERSION}")
        else ()
            message(STATUS "${PREFIX}: No git tag found, default version used")
            set(${PREFIX}_VERSION "0.0.0")
            set(${PREFIX}_VERSION_MAJOR "0")
            set(${PREFIX}_VERSION_MINOR "0")
            set(${PREFIX}_VERSION_PATCH "0")
        endif ()

        # Export to parent scope
        set(${PREFIX}_VERSION "${${PREFIX}_VERSION}" PARENT_SCOPE)
        set(${PREFIX}_VERSION_MAJOR "${${PREFIX}_VERSION_MAJOR}" PARENT_SCOPE)
        set(${PREFIX}_VERSION_MINOR "${${PREFIX}_VERSION_MINOR}" PARENT_SCOPE)
        set(${PREFIX}_VERSION_PATCH "${${PREFIX}_VERSION_PATCH}" PARENT_SCOPE)
        set(${PREFIX}_GIT_SHA1 "${${PREFIX}_GIT_SHA1}" PARENT_SCOPE)
    endif()
endfunction()

if (GIT_EXECUTABLE)
    # Get main project version
    get_git_version_info("${CMAKE_SOURCE_DIR}" "PROJECT")

    # Start building version.h content
    set(VERSION_H_CONTENT "#pragma once

// Main project version
#define PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR}
#define PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR}
#define PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH}
#define PROJECT_GIT_SHA1      \"${PROJECT_GIT_SHA1}\"

// Submodule versions (auto-generated from .gitmodules and FetchContent)
")
    
    set(PRINT_ALL_VERSIONS_BODY "    printf(\"=== Version Information ===\\r\\n\"); \\
    PRINT_VERSION(\"Current project\", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH, PROJECT_GIT_SHA1);")
    
    # Track processed dependencies to avoid duplicates
    set(PROCESSED_DEPS "")
    
    # Helper function to process a dependency
    macro(process_dependency DEP_PATH DEP_NAME)
        list(FIND PROCESSED_DEPS "${DEP_NAME}" ALREADY_PROCESSED)
        if(ALREADY_PROCESSED EQUAL -1 AND EXISTS "${DEP_PATH}/.git")
            string(TOUPPER ${DEP_NAME} SUBMODULE_PREFIX)
            string(REPLACE "-" "_" SUBMODULE_PREFIX ${SUBMODULE_PREFIX})
            
            get_git_version_info("${DEP_PATH}" "${SUBMODULE_PREFIX}")
            message(STATUS "Found dependency: ${DEP_NAME} -> ${SUBMODULE_PREFIX}")
            
            # Add defines for this dependency
            set(VERSION_H_CONTENT "${VERSION_H_CONTENT}
// ${DEP_NAME}
#define ${SUBMODULE_PREFIX}_VERSION_MAJOR ${${SUBMODULE_PREFIX}_VERSION_MAJOR}
#define ${SUBMODULE_PREFIX}_VERSION_MINOR ${${SUBMODULE_PREFIX}_VERSION_MINOR}
#define ${SUBMODULE_PREFIX}_VERSION_PATCH ${${SUBMODULE_PREFIX}_VERSION_PATCH}
#define ${SUBMODULE_PREFIX}_GIT_SHA1      \"${${SUBMODULE_PREFIX}_GIT_SHA1}\"
")
            
            # Add print statement
            set(PRINT_ALL_VERSIONS_BODY "${PRINT_ALL_VERSIONS_BODY} \\
    PRINT_VERSION(\"${DEP_NAME}\", ${SUBMODULE_PREFIX}_VERSION_MAJOR, ${SUBMODULE_PREFIX}_VERSION_MINOR, ${SUBMODULE_PREFIX}_VERSION_PATCH, ${SUBMODULE_PREFIX}_GIT_SHA1);")
            
            list(APPEND PROCESSED_DEPS "${DEP_NAME}")
        endif()
    endmacro()
    
    # 1. Process git submodules from .gitmodules
    if(EXISTS "${CMAKE_SOURCE_DIR}/.gitmodules")
        file(READ "${CMAKE_SOURCE_DIR}/.gitmodules" GITMODULES_CONTENT)
        string(REGEX MATCHALL "path = ([^\n]+)" SUBMODULE_PATHS "${GITMODULES_CONTENT}")
        
        foreach(PATH_LINE ${SUBMODULE_PATHS})
            string(REGEX REPLACE "path = (.+)" "\\1" SUBMODULE_PATH "${PATH_LINE}")
            string(STRIP "${SUBMODULE_PATH}" SUBMODULE_PATH)
            
            set(FULL_PATH "${CMAKE_SOURCE_DIR}/${SUBMODULE_PATH}")
            get_filename_component(SUBMODULE_NAME ${SUBMODULE_PATH} NAME)
            
            process_dependency("${FULL_PATH}" "${SUBMODULE_NAME}")
        endforeach()
    endif()
    
    # 2. Process FetchContent dependencies
    # FetchContent stores dependencies in CMAKE_BINARY_DIR/_deps
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps")
        file(GLOB FETCHCONTENT_DIRS "${CMAKE_BINARY_DIR}/_deps/*-src")
        foreach(FC_DIR ${FETCHCONTENT_DIRS})
            if(IS_DIRECTORY "${FC_DIR}")
                get_filename_component(FC_NAME ${FC_DIR} NAME)
                string(REGEX REPLACE "-src$" "" FC_NAME "${FC_NAME}")
                
                process_dependency("${FC_DIR}" "${FC_NAME}")
            endif()
        endforeach()
    endif()
    
    # 3. Support for manually registered external dependencies via cache variable
    # Usage in CMakeLists.txt: set(RHS_EXTERNAL_DEPS "path1:name1;path2:name2" CACHE STRING "External dependencies")
    if(DEFINED RHS_EXTERNAL_DEPS AND NOT "${RHS_EXTERNAL_DEPS}" STREQUAL "")
        foreach(EXT_DEP ${RHS_EXTERNAL_DEPS})
            string(REPLACE ":" ";" DEP_PARTS "${EXT_DEP}")
            list(LENGTH DEP_PARTS DEP_PARTS_LEN)
            if(DEP_PARTS_LEN EQUAL 2)
                list(GET DEP_PARTS 0 EXT_PATH)
                list(GET DEP_PARTS 1 EXT_NAME)
                
                process_dependency("${EXT_PATH}" "${EXT_NAME}")
            endif()
        endforeach()
    endif()

    # Add helper macros
    set(VERSION_H_CONTENT "${VERSION_H_CONTENT}
// Helper macro to print version info
#define PRINT_VERSION(name, major, minor, patch, sha) \\
    printf(\"%-12s %d.%d.%d (%s)\\r\\n\", name, major, minor, patch, sha)

// Macro to print all versions
#define PRINT_ALL_VERSIONS() \\
    do { \\
${PRINT_ALL_VERSIONS_BODY} \\
    } while(0)
")

    # Write the file
    file(WRITE ${CMAKE_BINARY_DIR}/generation/rhs_version.h "${VERSION_H_CONTENT}")

    include_directories(${CMAKE_BINARY_DIR}/generation)
else ()
    message(STATUS "Can't find Git SCM")
endif ()
