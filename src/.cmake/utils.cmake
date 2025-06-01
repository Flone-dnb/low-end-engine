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
    if (CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(${PROJECT_NAME} PRIVATE -static-libasan)
    endif()
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

function(create_symlink_to_res)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${PROJECT_NAME})
    file(CREATE_LINK
        ${CMAKE_CURRENT_LIST_DIR}/../../res
        ${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${PROJECT_NAME}/res
        SYMBOLIC
    )
endfunction()
