# Enable address sanitizer (gcc/clang only).
function(enable_address_sanitizer_for_target TARGET_NAME)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Address sanitizer is only supported for GCC/Clang.")
    endif()
    message(STATUS "Address sanitizer is enabled.")
    target_compile_definitions(${TARGET_NAME} PRIVATE ENGINE_ASAN_ENABLED)
    target_compile_options(${TARGET_NAME} PRIVATE -fno-omit-frame-pointer)
    target_compile_options(${TARGET_NAME} PRIVATE -fsanitize=address)
    target_link_libraries(${TARGET_NAME} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${TARGET_NAME} PRIVATE -fsanitize=address)
    if (CMAKE_COMPILER_IS_GNUCC)
        target_compile_options(${TARGET_NAME} PRIVATE -static-libasan)
    endif()
endfunction()

# Enable more warnings and warnings as errors.
function(enable_more_warnings_for_target TARGET_NAME)
    # More warnings.
    if(MSVC)
        target_compile_options(${TARGET_NAME} PUBLIC /W3 /WX)
    else()
        target_compile_options(${TARGET_NAME} PUBLIC -Wall -Wextra -Werror -Wconversion -Wno-unused-parameter -Wno-unused-variable)
    endif()

    # Compiler specific flags.
    if(MSVC)
        target_compile_options(${TARGET_NAME} PUBLIC /utf-8)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # False-positive in GCC for `ReflectedTypeDatabase::getTypeInfo`.
        target_compile_options(${TARGET_NAME} PUBLIC -Wno-dangling-reference)
    endif()
endfunction()

# Adds a program as a post build step to check that all Node derived types in override functions are "calling super"
# (base class implementation of the function being overridden). So that the programmer does not need to remember that.
function(add_node_super_call_checker_for_target TARGET_NAME GLOBAL_PATH_TO_NODES_PRIVATE GLOBAL_PATH_TO_NODES_PUBLIC)
    add_dependencies(${TARGET_NAME} ${SUPER_CALL_CHECKER_NAME})
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${SUPER_CALL_CHECKER_NAME}
            "${GLOBAL_PATH_TO_NODES_PRIVATE}"
            "${GLOBAL_PATH_TO_NODES_PUBLIC}"
    )
endfunction()

function(create_symlink_to_res)
    set(PROJ_BIN_DIR ${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${PROJECT_NAME})
    set(RES_SYMLINK_PATH ${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${PROJECT_NAME}/res)

    if(NOT EXISTS ${PROJ_BIN_DIR})
        file(MAKE_DIRECTORY ${PROJ_BIN_DIR})
    endif()
    
    if(NOT EXISTS ${RES_SYMLINK_PATH})
        message(STATUS "Creating symlink to `res` directory at ${RES_SYMLINK_PATH}")
        file(CREATE_LINK
            ${CMAKE_CURRENT_LIST_DIR}/../../res
            ${RES_SYMLINK_PATH}
            SYMBOLIC
        )
    endif()
endfunction()
