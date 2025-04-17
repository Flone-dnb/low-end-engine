cmake_minimum_required(VERSION 3.16)

project(editor_lib)

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

# Specify project sources.
set(PROJECT_SOURCES
    public/EditorGameInstance.h
    private/EditorGameInstance.cpp
    private/input/EditorInputEventIds.hpp
    public/node/EditorCameraNode.h
    private/node/EditorCameraNode.cpp
    public/misc/EditorNodeCreationHelpers.hpp
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
    add_node_super_call_checker(${CMAKE_CURRENT_LIST_DIR}/public/node ${CMAKE_CURRENT_LIST_DIR}/private/node)
endif()

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# Add engine library.
set(ENGINE_TARGET engine_lib)
message(STATUS "${PROJECT_NAME}: started adding ${ENGINE_TARGET}...\n----------------------------------------------\n")
add_subdirectory(../${ENGINE_TARGET} ${DEPENDENCY_BUILD_DIR_NAME}/${ENGINE_TARGET})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ENGINE_TARGET})
add_dependencies(${PROJECT_NAME} ${ENGINE_TARGET})
message(STATUS "\n\n----------------------------------------------\n${PROJECT_NAME}: finished adding ${ENGINE_TARGET}")
