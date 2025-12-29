# Set default build mode to be `Release`.
message(STATUS "${PROJECT_NAME}: build type: ${CMAKE_BUILD_TYPE}.")

# Prepare variable for build type.
if(CMAKE_BUILD_TYPE MATCHES "^[Rr]elease")
    set(IS_RELEASE_BUILD ON CACHE BOOL "" FORCE)
else()
    set(IS_RELEASE_BUILD OFF CACHE BOOL "" FORCE)
endif()

# Add `DEBUG` macro in debug builds.
if(NOT IS_RELEASE_BUILD)
    message(STATUS "${PROJECT_NAME}: adding DEBUG and ENGINE_DEBUG_TOOLS macros for this build type.")
    add_compile_definitions(DEBUG)
    add_compile_definitions(ENGINE_DEBUG_TOOLS)
endif()

# Define ENGINE_UI_ONLY macro.
if (ENGINE_UI_ONLY)
    message(STATUS "${PROJECT_NAME}: adding ENGINE_UI_ONLY macro.")
    add_compile_definitions(ENGINE_UI_ONLY)
endif()

# Add `WIN32` and `_WIN32` macros on Windows (some setups don't define them).
if(WIN32)
    add_compile_definitions(WIN32)
    add_compile_definitions(_WIN32)
endif()

# Define some names.
set(BUILD_DIRECTORY_NAME OUTPUT)
set(DEPENDENCY_BUILD_DIR_NAME dep)
set(SUPER_CALL_CHECKER_NAME node_super_call_checker)

# Enable cmake folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Define some folder names.
set(PROJECT_FOLDER "PROJECT")
set(EXTERNAL_FOLDER "EXTERNAL")
