cmake_minimum_required(VERSION 3.16)

project(low_end_engine)

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

# Include essential stuff.
include(src/.cmake/essential.cmake)

# Set default build mode.
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    # There's very little reason to use slow debug build type.
    message(FATAL_ERROR "Use RelWithDebInfo build type instead of the Debug build type.")
endif()
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

# Add node_super_call_checker.
set(NODE_SUPER_CALL_CHECKER_DIR node_super_call_checker)
message(STATUS "Adding ${NODE_SUPER_CALL_CHECKER_DIR}...")
add_subdirectory(src/${NODE_SUPER_CALL_CHECKER_DIR} ${BUILD_DIRECTORY_NAME}/${NODE_SUPER_CALL_CHECKER_DIR} SYSTEM)

# Add engine_lib.
set(ENGINE_LIB_DIR engine_lib)
message(STATUS "Adding ${ENGINE_LIB_DIR}...")
add_subdirectory(src/${ENGINE_LIB_DIR} ${BUILD_DIRECTORY_NAME}/${ENGINE_LIB_DIR})

# Add editor_lib.
set(EDITOR_LIB_DIR editor_lib)
message(STATUS "Adding ${EDITOR_LIB_DIR}...")
add_subdirectory(src/${EDITOR_LIB_DIR} ${BUILD_DIRECTORY_NAME}/${EDITOR_LIB_DIR})

# Add editor.
set(EDITOR_TARGET_DIR_NAME editor)
message(STATUS "Adding ${EDITOR_TARGET_DIR_NAME}...")
add_subdirectory(src/${EDITOR_TARGET_DIR_NAME} ${BUILD_DIRECTORY_NAME}/${EDITOR_TARGET_DIR_NAME})

# Add engine tests.
set(TESTS_TARGET_DIR_NAME engine_tests)
message(STATUS "Adding ${TESTS_TARGET_DIR_NAME}...")
add_subdirectory(src/${TESTS_TARGET_DIR_NAME} ${BUILD_DIRECTORY_NAME}/${TESTS_TARGET_DIR_NAME})

# Add game targets.
if (DEFINED GAME_PROJECT_NAME) # <- comes from engine_settings.cmake
    # Game directories should have been generated using our project manager.

    # Add game_lib.
    set(GAME_LIB_DIR ${GAME_PROJECT_NAME}_lib)
    message(STATUS "Adding ${GAME_LIB_DIR}...")
    add_subdirectory(src/${GAME_LIB_DIR} ${BUILD_DIRECTORY_NAME}/${GAME_LIB_DIR})

    # Add game.
    message(STATUS "Adding ${GAME_PROJECT_NAME}...")
    add_subdirectory(src/${GAME_PROJECT_NAME} ${BUILD_DIRECTORY_NAME}/${GAME_PROJECT_NAME})

    # Add game_tests.
    set(GAME_TESTS_DIR ${GAME_PROJECT_NAME}_tests)
    message(STATUS "Adding ${GAME_TESTS_DIR}...")
    add_subdirectory(src/${GAME_TESTS_DIR} ${BUILD_DIRECTORY_NAME}/${GAME_TESTS_DIR})
endif()
