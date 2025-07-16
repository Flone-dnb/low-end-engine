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
    src/EditorGameInstance.h
    src/EditorGameInstance.cpp
    src/EditorColorTheme.h
    src/EditorColorTheme.cpp
    src/input/EditorInputEventIds.hpp
    src/node/EditorCameraNode.h
    src/node/EditorCameraNode.cpp
    src/node/node_tree_inspector/NodeTreeInspector.h
    src/node/node_tree_inspector/NodeTreeInspector.cpp
    src/node/node_tree_inspector/NodeTreeInspectorItem.h
    src/node/node_tree_inspector/NodeTreeInspectorItem.cpp
    src/node/property_inspector/PropertyInspector.h
    src/node/property_inspector/PropertyInspector.cpp
    src/node/property_inspector/GlmVecInspector.h
    src/node/property_inspector/GlmVecInspector.cpp
    src/node/menu/ContextMenuNode.h
    src/node/menu/ContextMenuNode.cpp
    src/node/menu/SelectNodeTypeMenu.h
    src/node/menu/SelectNodeTypeMenu.cpp
    src/node/menu/SetNameMenu.h
    src/node/menu/SetNameMenu.cpp
    src/node/LogViewNode.h
    src/node/LogViewNode.cpp
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
target_include_directories(${PROJECT_NAME} PUBLIC src)

# Debug helper.
if (NOT IS_RELEASE_BUILD AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/debug.cmake")
    message(STATUS "Disabling optimizations because the file \"debug.cmake\" exists.")
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE -Od)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O0)
    endif()
endif()

# -------------------------------------------------------------------------------------------------
#                                           TOOLS
# -------------------------------------------------------------------------------------------------

# asan
if(NOT IS_RELEASE_BUILD AND NOT WIN32)
    enable_address_sanitizer()
endif()

add_node_super_call_checker(${CMAKE_CURRENT_LIST_DIR}/src/node ${CMAKE_CURRENT_LIST_DIR}/src/node)

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# Add engine library.
set(ENGINE_LIB_TARGET engine_lib)
if (NOT TARGET ${ENGINE_LIB_TARGET}) # define target only if not defined yet
    message(STATUS "${PROJECT_NAME}: started adding ${ENGINE_LIB_TARGET}...\n----------------------------------------------\n")
    add_subdirectory(../${ENGINE_LIB_TARGET} ${DEPENDENCY_BUILD_DIR_NAME}/${ENGINE_LIB_TARGET})
    message(STATUS "\n\n----------------------------------------------\n${PROJECT_NAME}: finished adding ${ENGINE_LIB_TARGET}")
else()
    message(STATUS "${PROJECT_NAME}: ${ENGINE_LIB_TARGET} already defined, just using it without redefining")
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC ${ENGINE_LIB_TARGET})
add_dependencies(${PROJECT_NAME} ${ENGINE_LIB_TARGET})
