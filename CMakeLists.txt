cmake_minimum_required(VERSION 3.16)

project(low_end_engine)

option(DISABLE_DEV_STUFF "Enable to disable things like tests, clang-tidy and other dev tools." OFF)
option(ENGINE_UI_ONLY "Enable in case you're making a UI-only app, this way the app will be slightly more performant and will use slightly less RAM." OFF)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/engine_settings.cmake")
    # This file is used to override global engine cmake options.
    message(STATUS "Including engine settings from \"engine_settings.cmake\"")
    include(engine_settings.cmake)
endif()

if (ENGINE_UI_ONLY)
    message(STATUS "ENGINE_UI_ONLY enabled.")
else()
    message(STATUS "ENGINE_UI_ONLY disabled.")
endif()

set(BUILD_DIRECTORY_NAME OUTPUT)

# Set default build mode.
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    # There's very little reason to use slow debug build type.
    message(FATAL_ERROR "Use RelWithDebInfo build type instead of the Debug build type.")
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

message(STATUS "Adding executable targets...")

set(TESTS_TARGET_DIR_NAME engine_tests)
if (GAME_PROJECT_NAME) # <- comes from engine_settings.cmake
    # Game directories should have been generated using our project manager.
    set(TESTS_TARGET_DIR_NAME ${GAME_PROJECT_NAME}_tests)

    # Add game target.
    set(GAME_TARGET_DIR_NAME ${GAME_PROJECT_NAME})
    message(STATUS "Adding target ${GAME_TARGET_DIR_NAME}...")
    add_subdirectory(src/${GAME_TARGET_DIR_NAME} ${BUILD_DIRECTORY_NAME}/${GAME_TARGET_DIR_NAME})
endif()

# Add editor target.
set(EDITOR_TARGET_DIR_NAME editor)
message(STATUS "Adding target ${EDITOR_TARGET_DIR_NAME}...")
add_subdirectory(src/${EDITOR_TARGET_DIR_NAME} ${BUILD_DIRECTORY_NAME}/${EDITOR_TARGET_DIR_NAME})

# Add tests target.
if (NOT DISABLE_DEV_STUFF)
    message(STATUS "Adding target ${TESTS_TARGET_DIR_NAME}...")
    add_subdirectory(src/${TESTS_TARGET_DIR_NAME} ${BUILD_DIRECTORY_NAME}/${TESTS_TARGET_DIR_NAME})
endif()
