cmake_minimum_required(VERSION 3.16)

project(low_end_engine)

option(DISABLE_DEV_STUFF "Enable to disable things like tests, clang-tidy and other dev tools." OFF)
option(ENGINE_UI_ONLY "Enable in case you're making a UI-only app, this way the app will be slightly more performant and will use slightly less RAM." OFF)

set(BUILD_DIRECTORY_NAME OUTPUT)

# Set default build mode.
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    # There's very little reason to use slow debug build type.
    message(FATAL_ERROR "Use RelWithDebInfo build type instead of the Debug build type.")
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# Add executable targets.
message(STATUS "Adding executable targets...")

# Add editor target.
set(EDITOR_DIRECTORY editor)
message(STATUS "Adding target ${EDITOR_DIRECTORY}...")
add_subdirectory(src/${EDITOR_DIRECTORY} ${BUILD_DIRECTORY_NAME}/${EDITOR_DIRECTORY})

# Add tests target.
if (NOT DISABLE_DEV_STUFF)
    set(ENGINE_TESTS_DIRECTORY engine_tests)
    message(STATUS "Adding target ${ENGINE_TESTS_DIRECTORY}...")
    add_subdirectory(src/${ENGINE_TESTS_DIRECTORY} ${BUILD_DIRECTORY_NAME}/${ENGINE_TESTS_DIRECTORY})
endif()
