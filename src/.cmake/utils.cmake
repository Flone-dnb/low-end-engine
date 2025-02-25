# Enable address sanitizer (gcc/clang only).
function(enable_address_sanitizer)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Address sanitizer is only supported for GCC/Clang.")
    endif()
    message(STATUS "Address sanitizer is enabled.")
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-omit-frame-pointer)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    target_link_libraries(${PROJECT_NAME} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${PROJECT_NAME} PRIVATE -fsanitize=address)
endfunction()

# Enable more warnings and warnings as errors.
function(enable_more_warnings)
    # More warnings.
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PUBLIC /W3 /WX)
    else()
        target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable)
    endif()

    # Compiler specific flags.
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PUBLIC /utf-8)
    endif()
endfunction()

# Enable Doxygen to be run before building your target.
function(enable_doxygen DOCS_DIRECTORY)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_TARGET_NAME ${PROJECT_NAME}_doxygen)
    add_custom_target(${DOXYGEN_TARGET_NAME}
        COMMAND doxygen
        WORKING_DIRECTORY ${DOCS_DIRECTORY}
        COMMENT "${PROJECT_NAME}: generating documentation using Doxygen..."
        VERBATIM)
    set_target_properties(${DOXYGEN_TARGET_NAME} PROPERTIES FOLDER ${EXTERNAL_FOLDER})
    add_dependencies(${PROJECT_NAME} ${DOXYGEN_TARGET_NAME})
    message(STATUS "${PROJECT_NAME}: doxygen is enabled.")
endfunction()

# Enables clang-tidy.
function(enable_clang_tidy CLANG_TIDY_CONFIG_PATH)
    find_program (CLANG_TIDY NAMES "clang-tidy" REQUIRED)
    set(RUN_CLANG_TIDY
        "${CLANG_TIDY}"
        "--config-file=${CLANG_TIDY_CONFIG_PATH}")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${RUN_CLANG_TIDY}")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PUBLIC -EHsc) # needed for clang-tidy when running MSVC to enable exceptions
    endif()
    message(STATUS "${PROJECT_NAME}: Clang-tidy is enabled.")
endfunction()

# Adds a program as a post build step to check that all Node derived types in override functions are "calling super"
# (base class implementation of the function being overridden). So that the programmer does not need to remember that.
function(add_node_super_call_checker GLOBAL_PATH_TO_NODES_PRIVATE GLOBAL_PATH_TO_NODES_PUBLIC)
    add_subdirectory("../${SUPER_CALL_CHECKER_NAME}" ${DEPENDENCY_BUILD_DIR_NAME}/${SUPER_CALL_CHECKER_NAME} SYSTEM)
    add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_${SUPER_CALL_CHECKER_NAME})
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        WORKING_DIRECTORY "${DEPENDENCY_BUILD_DIR_NAME}/${SUPER_CALL_CHECKER_NAME}"
        COMMAND ${PROJECT_NAME}_${SUPER_CALL_CHECKER_NAME}
            "${GLOBAL_PATH_TO_NODES_PRIVATE}"
            "${GLOBAL_PATH_TO_NODES_PUBLIC}"
    )
endfunction()
