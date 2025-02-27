cmake_minimum_required(VERSION 3.16)

project(engine_lib)

# Make sure we are running a 64-bit system.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "We don't support 32 bit systems.")
endif()

# Define some relative paths.
set(RELATIVE_EXT_PATH "../../ext")
set(RELATIVE_CMAKE_HELPERS_PATH "../.cmake")

# Include essential stuff.
include(${RELATIVE_CMAKE_HELPERS_PATH}/essential.cmake)

# Include helper functions.
include(${RELATIVE_CMAKE_HELPERS_PATH}/utils.cmake)

# -------------------------------------------------------------------------------------------------
#                                          TARGET SOURCES
# -------------------------------------------------------------------------------------------------

# Sources.
set(PROJECT_SOURCES
    public/render/Renderer.h
    private/render/Renderer.cpp
    private/game/Window.cpp
    public/game/Window.h
    private/render/SdlManager.hpp
    private/misc/Error.cpp
    public/misc/Error.h
    public/misc/Globals.h
    private/misc/Globals.cpp
    private/misc/ProjectPaths.cpp
    public/misc/ProjectPaths.h
    public/math/GLMath.hpp
    public/math/MathHelpers.hpp
    public/io/Logger.h
    private/io/Logger.cpp
    public/io/ConfigManager.h
    private/io/ConfigManager.cpp
    private/game/GameManager.cpp
    private/game/GameManager.h
    public/game/GameInstance.h
    private/game/GameInstance.cpp
    private/game/World.cpp
    private/game/World.h
    private/game/node/Node.cpp
    public/game/node/Node.h
    private/game/node/SpatialNode.cpp
    public/game/node/SpatialNode.h
    public/game/node/NodeTickGroup.hpp
    public/input/InputManager.h
    public/input/KeyboardButton.hpp
    public/input/MouseButton.hpp
    public/input/GamepadButton.hpp
    private/input/InputManager.cpp
    # add your .h/.cpp files here
)

# Define target.
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES})

# -------------------------------------------------------------------------------------------------
#                                         CONFIGURE TARGET
# -------------------------------------------------------------------------------------------------

# Set target folder.
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_FOLDER})

# Enable more warnings and warnings as errors.
enable_more_warnings()

# Set C++ standard.
set(PROJECT_CXX_STANDARD_VERSION 20)
set(CMAKE_CXX_STANDARD ${PROJECT_CXX_STANDARD_VERSION})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_${PROJECT_CXX_STANDARD_VERSION})
message(STATUS "${PROJECT_NAME}: using the following C++ standard: ${CMAKE_CXX_STANDARD}")

# Add includes.
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${RELATIVE_EXT_PATH})
target_include_directories(${PROJECT_NAME} PUBLIC public)
target_include_directories(${PROJECT_NAME} PUBLIC private)

# -------------------------------------------------------------------------------------------------
#                                           TOOLS
# -------------------------------------------------------------------------------------------------

# asan
if(NOT IS_RELEASE_BUILD AND NOT WIN32 AND NOT DISABLE_DEV_STUFF)
    enable_address_sanitizer()
endif()

# clang-tidy
if (IS_RELEASE_BUILD AND NOT DISABLE_DEV_STUFF)
    enable_clang_tidy(${CMAKE_CURRENT_LIST_DIR}/../../.clang-tidy)
endif()

# node super call checker
if (NOT DISABLE_DEV_STUFF)
    add_node_super_call_checker(${CMAKE_CURRENT_LIST_DIR}/public/game/node ${CMAKE_CURRENT_LIST_DIR}/private/game/node)
endif()

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# Add SDL.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SDL\"...")
# Set path for SDL libraries.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${DLL_DIRECTORY_TO_COPY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${BUILD_DIRECTORY_NAME}/${DLL_DIRECTORY_TO_COPY}")
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${RELATIVE_EXT_PATH}/SDL/include)
add_subdirectory(${RELATIVE_EXT_PATH}/SDL ${DEPENDENCY_BUILD_DIR_NAME}/SDL EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2)
set_target_properties(SDL2 PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add GLAD.
message(STATUS "${PROJECT_NAME}: adding external dependency \"GLAD\"...")
set(GLAD_TARGET_NAME glad)
add_library(${GLAD_TARGET_NAME} STATIC
    ${RELATIVE_EXT_PATH}/glad_gles31/src/glad.c
    ${RELATIVE_EXT_PATH}/glad_gles31/include/glad/glad.h
)
target_include_directories(${GLAD_TARGET_NAME} SYSTEM PUBLIC ${RELATIVE_EXT_PATH}/glad_gles31/include)
target_link_libraries(${PROJECT_NAME} PUBLIC ${GLAD_TARGET_NAME})
add_dependencies(${PROJECT_NAME} ${GLAD_TARGET_NAME})
set_target_properties(${GLAD_TARGET_NAME} PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add GLM.
message(STATUS "${PROJECT_NAME}: adding external dependency \"GLM\"...")
add_subdirectory(${RELATIVE_EXT_PATH}/glm ${DEPENDENCY_BUILD_DIR_NAME}/glm SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC glm)
set_target_properties(glm PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add spdlog.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SPDLOG\"...")
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/spdlog ${DEPENDENCY_BUILD_DIR_NAME}/spdlog SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
set_target_properties(spdlog PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add toml11.
message(STATUS "${PROJECT_NAME}: adding external dependency \"TOML11\"...")
if (MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /Zc:__cplusplus)
endif()
add_subdirectory(${RELATIVE_EXT_PATH}/toml11 ${DEPENDENCY_BUILD_DIR_NAME}/toml11 SYSTEM)
set_target_properties(toml11 PROPERTIES FOLDER ${EXTERNAL_FOLDER})
