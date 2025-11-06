cmake_minimum_required(VERSION 3.16)

project(low_end_engine)

option(ENGINE_UI_ONLY "Enable in case you're making a UI-only app, this way the app will be slightly more performant and will use slightly less RAM." OFF)
option(ENGINE_ENABLE_DEBUG_TOOLS "Can be useful in Release builds to measure performance" OFF)

# Check build mode.
if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
    # There's very little reason to use slow debug build type.
    message(FATAL_ERROR "Use RelWithDebInfo build type instead of the Debug build type for development.")
endif()
if (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Build type is not defined.")
endif()

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/engine_settings.cmake")
    # This file is used to override global engine cmake options.
    message(STATUS "Including engine settings from \"engine_settings.cmake\"")
    include(engine_settings.cmake)
endif()

# Include essential stuff.
include(src/.cmake/essential.cmake)

if (ENGINE_UI_ONLY)
    message(STATUS "ENGINE_UI_ONLY enabled.")
else()
    message(STATUS "ENGINE_UI_ONLY disabled.")
endif()

if (ENGINE_ENABLE_DEBUG_TOOLS AND IS_RELEASE_BUILD)
    message(WARNING "Note: you enabled ENGINE_DEBUG_TOOLS in release build.")
    add_compile_definitions(ENGINE_DEBUG_TOOLS)
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
    set(GAME_LIB_DIR game_lib)
    message(STATUS "Adding ${GAME_LIB_DIR}...")
    add_subdirectory(src/${GAME_LIB_DIR} ${BUILD_DIRECTORY_NAME}/${GAME_LIB_DIR})

    # Add game.
    set(GAME_DIR game)
    message(STATUS "Adding ${GAME_DIR}...")
    add_subdirectory(src/${GAME_DIR} ${BUILD_DIRECTORY_NAME}/${GAME_DIR})

    # Add game_tests.
    set(GAME_TESTS_DIR game_tests)
    message(STATUS "Adding ${GAME_TESTS_DIR}...")
    add_subdirectory(src/${GAME_TESTS_DIR} ${BUILD_DIRECTORY_NAME}/${GAME_TESTS_DIR})
endif()
