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
    private/game/Window.cpp
    public/game/Window.h
    public/render/Renderer.h
    private/render/Renderer.cpp
    private/render/PhysicsDebugDrawer.hpp
    private/material/Material.cpp
    public/material/Material.h
    private/material/TextureManager.cpp
    public/material/TextureManager.h
    private/material/TextureHandle.cpp
    public/material/TextureHandle.h
    public/material/TextureUsage.hpp
    private/render/RenderStatistics.cpp
    public/render/RenderStatistics.h
    private/render/GpuResourceManager.cpp
    private/render/GpuResourceManager.h
    public/render/ShaderManager.h
    private/render/shader/ShaderManager.cpp
    private/render/shader/Shader.h
    private/render/shader/Shader.cpp
    private/render/shader/LightSourceShaderArray.cpp
    private/render/shader/LightSourceShaderArray.h
    private/render/shader/ShaderArrayIndexManager.cpp
    private/render/shader/ShaderArrayIndexManager.h
    private/render/wrapper/ShaderProgram.cpp
    private/render/wrapper/ShaderProgram.h
    private/render/wrapper/VertexArrayObject.cpp
    private/render/wrapper/VertexArrayObject.h
    private/render/wrapper/Framebuffer.cpp
    private/render/wrapper/Framebuffer.h
    private/render/wrapper/Buffer.cpp
    private/render/wrapper/Buffer.h
    private/render/wrapper/Texture.cpp
    private/render/wrapper/Texture.h
    public/render/ShaderConstantsSetter.hpp
    private/render/LightSourceManager.cpp
    private/render/LightSourceManager.h
    private/render/UiNodeManager.cpp
    private/render/UiNodeManager.h
    private/render/MeshRenderer.h
    private/render/MeshRenderer.cpp
    private/render/RenderingHandle.h
    private/render/RenderingHandle.cpp
    public/render/ShaderAlignmentConstants.hpp
    private/render/FontManager.cpp
    public/render/FontManager.h
    public/render/PostProcessManager.h
    public/render/UiLayer.hpp
    private/render/PostProcessManager.cpp
    public/render/DebugDrawer.h
    private/render/DebugDrawer.cpp
    private/misc/InitManager.hpp
    private/misc/Error.cpp
    public/misc/Error.h
    public/misc/Globals.h
    private/misc/Globals.cpp
    private/misc/ProjectPaths.cpp
    public/misc/ProjectPaths.h
    private/misc/ReflectedTypeDatabase.cpp
    public/misc/ReflectedTypeDatabase.h
    private/misc/ThreadPool.cpp
    private/misc/ThreadPool.h
    public/misc/Profiler.hpp
    public/misc/MemoryUsage.hpp
    public/sound/SoundChannel.hpp
    public/sound/SoundManager.h
    private/sound/SoundManager.cpp
    public/game/geometry/shapes/Frustum.h
    private/game/geometry/shapes/Frustum.cpp
    public/game/geometry/shapes/AABB.h
    private/game/geometry/shapes/AABB.cpp
    public/game/geometry/shapes/Cone.h
    private/game/geometry/shapes/Cone.cpp
    public/game/geometry/shapes/Plane.h
    private/game/geometry/shapes/Plane.cpp
    public/game/geometry/shapes/Sphere.h
    private/game/geometry/shapes/Sphere.cpp
    public/game/geometry/shapes/CollisionShape.h
    private/game/geometry/shapes/CollisionShape.cpp
    private/game/geometry/MeshNodeGeometry.cpp
    public/game/geometry/MeshNodeGeometry.h
    private/game/geometry/SkeletalMeshNodeGeometry.cpp
    public/game/geometry/SkeletalMeshNodeGeometry.h
    private/game/geometry/ConvexShapeGeometry.cpp
    public/game/geometry/ConvexShapeGeometry.h
    public/game/geometry/PrimitiveMeshGenerator.h
    private/game/geometry/PrimitiveMeshGenerator.cpp
    private/game/geometry/ScreenQuadGeometry.cpp
    public/game/geometry/ScreenQuadGeometry.h
    private/game/geometry/MeshIndexType.hpp
    private/game/script/ScriptManager.cpp
    public/game/script/ScriptManager.h
    public/game/script/Script.h
    private/game/script/Script.cpp
    private/game/script/ScriptFuncInterface.cpp
    public/game/script/ScriptFuncInterface.h
    public/game/script/ScriptTypeRegistration.hpp
    public/math/GLMath.hpp
    public/math/MathHelpers.hpp
    public/io/Log.h
    private/io/Log.cpp
    public/io/ConfigManager.h
    private/io/ConfigManager.cpp
    private/io/Serializable.cpp
    public/io/Serializable.h
    public/io/GltfImporter.h
    private/io/GltfImporter.cpp
    private/game/GameManager.cpp
    private/game/GameManager.h
    public/game/GameInstance.h
    private/game/GameInstance.cpp
    private/game/World.cpp
    public/game/World.h
    private/game/DebugConsole.cpp
    public/game/DebugConsole.h
    private/game/physics/CoordinateConversions.hpp
    private/game/physics/PhysicsManager.cpp
    private/game/physics/PhysicsManager.h
    private/game/physics/PhysicsLayers.cpp
    private/game/physics/PhysicsLayers.h
    private/game/node/Node.cpp
    public/game/node/Node.h
    private/game/node/SpatialNode.cpp
    public/game/node/SpatialNode.h
    public/game/node/NodeTickGroup.hpp
    public/game/node/CameraNode.h
    private/game/node/CameraNode.cpp
    public/game/node/MeshNode.h
    private/game/node/MeshNode.cpp
    public/game/node/SkeletalMeshNode.h
    private/game/node/SkeletalMeshNode.cpp
    public/game/node/Sound2dNode.h
    private/game/node/Sound2dNode.cpp
    public/game/node/Sound3dNode.h
    private/game/node/Sound3dNode.cpp
    public/game/node/SkeletonNode.h
    private/game/node/SkeletonNode.cpp
    private/game/node/SkeletonBoneAttachmentNode.cpp
    public/game/node/SkeletonBoneAttachmentNode.h
    private/game/node/physics/CollisionNode.cpp
    public/game/node/physics/CollisionNode.h
    private/game/node/physics/SimulatedBodyNode.cpp
    public/game/node/physics/SimulatedBodyNode.h
    private/game/node/physics/MovingBodyNode.cpp
    public/game/node/physics/MovingBodyNode.h
    private/game/node/physics/CharacterBodyNode.cpp
    public/game/node/physics/CharacterBodyNode.h
    private/game/node/physics/SimpleCharacterBodyNode.cpp
    public/game/node/physics/SimpleCharacterBodyNode.h
    private/game/node/physics/CompoundCollisionNode.cpp
    public/game/node/physics/CompoundCollisionNode.h
    private/game/node/physics/TriggerVolumeNode.cpp
    public/game/node/physics/TriggerVolumeNode.h
    public/game/node/light/DirectionalLightNode.h
    private/game/node/light/DirectionalLightNode.cpp
    public/game/node/light/SpotlightNode.h
    private/game/node/light/SpotlightNode.cpp
    public/game/node/light/PointLightNode.h
    private/game/node/light/PointLightNode.cpp
    private/game/node/ui/UiNode.cpp
    public/game/node/ui/UiNode.h
    private/game/node/ui/TextUiNode.cpp
    public/game/node/ui/TextUiNode.h
    private/game/node/ui/TextEditUiNode.cpp
    public/game/node/ui/TextEditUiNode.h
    private/game/node/ui/RectUiNode.cpp
    public/game/node/ui/RectUiNode.h
    private/game/node/ui/ButtonUiNode.cpp
    public/game/node/ui/ButtonUiNode.h
    private/game/node/ui/LayoutUiNode.cpp
    public/game/node/ui/LayoutUiNode.h
    private/game/node/ui/SliderUiNode.cpp
    public/game/node/ui/SliderUiNode.h
    private/game/node/ui/CheckboxUiNode.cpp
    public/game/node/ui/CheckboxUiNode.h
    private/game/node/ui/ProgressBarUiNode.cpp
    public/game/node/ui/ProgressBarUiNode.h
    private/game/camera/CameraManager.cpp
    private/game/camera/CameraProperties.cpp
    public/game/camera/CameraManager.h
    public/game/camera/CameraProperties.h
    public/input/InputManager.h
    public/input/KeyboardButton.hpp
    public/input/MouseButton.hpp
    public/input/GamepadButton.hpp
    private/input/InputManager.cpp
    # add your .h/.cpp files here
)

# Define targets (one without any specific macros and another for compilation with editor).
set(ENGINE_LIB_TARGET engine_lib)
set(ENGINE_LIB_TARGET_FOR_EDITOR engine_lib_for_editor)
add_library(${ENGINE_LIB_TARGET} STATIC ${PROJECT_SOURCES})
add_library(${ENGINE_LIB_TARGET_FOR_EDITOR} STATIC ${PROJECT_SOURCES})

# Define editor macros for editor's version of the engine_lib.
target_compile_definitions(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ENGINE_EDITOR)
if (IS_RELEASE_BUILD)
    # The editor needs debug drawing and debug console even in release builds.
    target_compile_definitions(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ENGINE_DEBUG_TOOLS)
endif()

# -------------------------------------------------------------------------------------------------
#                                         CONFIGURE TARGET
# -------------------------------------------------------------------------------------------------

# Set target folder.
set_target_properties(${ENGINE_LIB_TARGET} PROPERTIES FOLDER ${PROJECT_FOLDER})
set_target_properties(${ENGINE_LIB_TARGET_FOR_EDITOR} PROPERTIES FOLDER ${PROJECT_FOLDER})

# Enable more warnings and warnings as errors.
enable_more_warnings_for_target(${ENGINE_LIB_TARGET})
enable_more_warnings_for_target(${ENGINE_LIB_TARGET_FOR_EDITOR})

# Set C++ standard.
set(PROJECT_CXX_STANDARD_VERSION 20)
set(CMAKE_CXX_STANDARD ${PROJECT_CXX_STANDARD_VERSION})
target_compile_features(${ENGINE_LIB_TARGET} PUBLIC cxx_std_${PROJECT_CXX_STANDARD_VERSION})
target_compile_features(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC cxx_std_${PROJECT_CXX_STANDARD_VERSION})
message(STATUS "${PROJECT_NAME}: using the following C++ standard: ${CMAKE_CXX_STANDARD}")

# Add includes.
target_include_directories(${ENGINE_LIB_TARGET} SYSTEM PUBLIC ${RELATIVE_EXT_PATH})
target_include_directories(${ENGINE_LIB_TARGET} PUBLIC public)
target_include_directories(${ENGINE_LIB_TARGET} PUBLIC private)

target_include_directories(${ENGINE_LIB_TARGET_FOR_EDITOR} SYSTEM PUBLIC ${RELATIVE_EXT_PATH})
target_include_directories(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC public)
target_include_directories(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC private)

# Debug helper.
if (NOT IS_RELEASE_BUILD AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/debug.cmake")
    message(STATUS "Disabling optimizations because the file \"debug.cmake\" exists.")
    if (MSVC)
        target_compile_options(${ENGINE_LIB_TARGET} PRIVATE -Od)
        target_compile_options(${ENGINE_LIB_TARGET_FOR_EDITOR} PRIVATE -Od)
    else()
        target_compile_options(${ENGINE_LIB_TARGET} PRIVATE -O0)
        target_compile_options(${ENGINE_LIB_TARGET_FOR_EDITOR} PRIVATE -O0)
    endif()
endif()

# -------------------------------------------------------------------------------------------------
#                                           TOOLS
# -------------------------------------------------------------------------------------------------

# asan
if(NOT IS_RELEASE_BUILD AND NOT WIN32)
    enable_address_sanitizer_for_target(${ENGINE_LIB_TARGET})
    enable_address_sanitizer_for_target(${ENGINE_LIB_TARGET_FOR_EDITOR})
endif()

add_node_super_call_checker_for_target(${ENGINE_LIB_TARGET} ${CMAKE_CURRENT_LIST_DIR}/public/game/node ${CMAKE_CURRENT_LIST_DIR}/private/game/node)
add_node_super_call_checker_for_target(${ENGINE_LIB_TARGET_FOR_EDITOR} ${CMAKE_CURRENT_LIST_DIR}/public/game/node ${CMAKE_CURRENT_LIST_DIR}/private/game/node)

# -------------------------------------------------------------------------------------------------
#                                       DEPENDENCIES
# -------------------------------------------------------------------------------------------------

# Group all dependencies into a single interface library, later we will link it to 2 version of the engine_lib
# one for the editor (with some editor macros) and one for the game.
set(ENGINE_LIB_DEPS_TARGET engine_lib_deps)
add_library(${ENGINE_LIB_DEPS_TARGET} INTERFACE)

# Add SDL.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SDL\"...")
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE) # we don't use SDL audio
target_include_directories(${ENGINE_LIB_DEPS_TARGET} SYSTEM INTERFACE ${RELATIVE_EXT_PATH}/SDL/include)
add_subdirectory(${RELATIVE_EXT_PATH}/SDL ${DEPENDENCY_BUILD_DIR_NAME}/SDL EXCLUDE_FROM_ALL)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE SDL3::SDL3)
set_target_properties(SDL3-static PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add GLAD.
message(STATUS "${PROJECT_NAME}: adding external dependency \"GLAD\"...")
set(GLAD_TARGET_NAME glad)
add_library(${GLAD_TARGET_NAME} STATIC
    ${RELATIVE_EXT_PATH}/glad_gles31/src/glad.c
    ${RELATIVE_EXT_PATH}/glad_gles31/include/KHR/khrplatform.h
    ${RELATIVE_EXT_PATH}/glad_gles31/include/glad/glad.h
)
target_include_directories(${GLAD_TARGET_NAME} SYSTEM PUBLIC ${RELATIVE_EXT_PATH}/glad_gles31/include)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE ${GLAD_TARGET_NAME})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} ${GLAD_TARGET_NAME})
set_target_properties(${GLAD_TARGET_NAME} PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add GLM.
message(STATUS "${PROJECT_NAME}: adding external dependency \"GLM\"...")
add_subdirectory(${RELATIVE_EXT_PATH}/glm ${DEPENDENCY_BUILD_DIR_NAME}/glm SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE glm)
set_target_properties(glm PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add spdlog.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SPDLOG\"...")
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/spdlog ${DEPENDENCY_BUILD_DIR_NAME}/spdlog SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE spdlog::spdlog)
set_target_properties(spdlog PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add toml11.
message(STATUS "${PROJECT_NAME}: adding external dependency \"TOML11\"...")
if (MSVC)
    target_compile_options(${ENGINE_LIB_DEPS_TARGET} INTERFACE /Zc:__cplusplus)
endif()
add_subdirectory(${RELATIVE_EXT_PATH}/toml11 ${DEPENDENCY_BUILD_DIR_NAME}/toml11 SYSTEM)
set_target_properties(toml11 PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add nameof.
target_include_directories(${ENGINE_LIB_DEPS_TARGET} INTERFACE ${RELATIVE_EXT_PATH}/nameof/include)

# Add FreeType.
message(STATUS "${PROJECT_NAME}: adding external dependency \"FreeType\"...")
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/freetype ${DEPENDENCY_BUILD_DIR_NAME}/freetype SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE freetype)
set_target_properties(freetype PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add tinygltf.
message(STATUS "${PROJECT_NAME}: adding external dependency \"tinygltf\"...")
set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/tinygltf ${DEPENDENCY_BUILD_DIR_NAME}/tinygltf SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE tinygltf)
set_target_properties(tinygltf PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} tinygltf)

# Add fpng.
message(STATUS "${PROJECT_NAME}: adding external dependency \"fpng\"...")
add_library(fpng
    ${RELATIVE_EXT_PATH}/fpng/src/fpng.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/lodepng.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/pvpngreader.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/fpng.h
)
if (NOT MSVC)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        target_compile_definitions(fpng PRIVATE FPNG_NO_SSE=1)
    else()
        target_compile_options(fpng PRIVATE -msse4.1 -mpclmul)
        target_compile_definitions(fpng PRIVATE FPNG_NO_SSE=0)
    endif()
endif()
target_include_directories(${ENGINE_LIB_DEPS_TARGET} INTERFACE ${RELATIVE_EXT_PATH}/fpng/src/)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE fpng)
set_target_properties(fpng PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} fpng)

# Add SFML.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SFML\"...")
set(SFML_BUILD_WINDOW OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_STATIC_LIBRARIES ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/SFML ${DEPENDENCY_BUILD_DIR_NAME}/SFML SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE sfml-audio)
set_target_properties(sfml-audio PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} sfml-audio)

# Add utfconv.
message(STATUS "${PROJECT_NAME}: adding external dependency \"utfconv\"...")
add_subdirectory(${RELATIVE_EXT_PATH}/utfconv ${DEPENDENCY_BUILD_DIR_NAME}/utfconv SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE utfconv)
set_target_properties(utfconv PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} utfconv)

# ozz-animation
message(STATUS "${PROJECT_NAME}: adding external dependency \"ozz-animation\"...")
set(ozz_build_fbx OFF CACHE BOOL "" FORCE)
set(ozz_build_samples OFF CACHE BOOL "" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "" FORCE)
set(ozz_build_tests OFF CACHE BOOL "" FORCE)
set(ozz_build_msvc_rt_dll ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/ozz-animation ${DEPENDENCY_BUILD_DIR_NAME}/ozz-animation SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE ozz_animation)
set_target_properties(ozz_animation PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} ozz_animation)
add_dependencies(${ENGINE_LIB_DEPS_TARGET} gltf2ozz)
set(GLTF2OZZ_BIN_PATH ${PROJECT_BINARY_DIR}/${DEPENDENCY_BUILD_DIR_NAME}/ozz-animation/src/animation/offline/gltf/gltf2ozz${CMAKE_EXECUTABLE_SUFFIX})
message(STATUS "gltf2ozz will be located at: ${GLTF2OZZ_BIN_PATH}")
add_compile_definitions(GLTF2OZZ_BIN_PATH="${GLTF2OZZ_BIN_PATH}")

# Add JoltPhysics.
message(STATUS "${PROJECT_NAME}: adding external dependency \"JoltPhysics\"...")
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(DISABLE_CUSTOM_ALLOCATOR ON CACHE BOOL "" FORCE)
if (NOT IS_RELEASE_BUILD)
    set(GENERATE_DEBUG_SYMBOLS ON CACHE BOOL "" FORCE)
else()
    set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
endif()
set(DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(CPP_EXCEPTIONS_ENABLED ON CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
if (WIN32)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
endif()
set(OBJECT_LAYER_BITS 16)
set(USE_SSE4_1 ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(USE_F16C OFF CACHE BOOL "" FORCE)  # <- 
set(USE_LZCNT OFF CACHE BOOL "" FORCE) # <- if enabling also uncomment
set(USE_TZCNT OFF CACHE BOOL "" FORCE) # <- checks in the PhysicsManager
set(USE_FMADD OFF CACHE BOOL "" FORCE) # <- 
set(USE_AVX OFF CACHE BOOL "" FORCE)
set(USE_AVX2 OFF CACHE BOOL "" FORCE)
set(USE_AVX512 OFF CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/JoltPhysics/Build ${DEPENDENCY_BUILD_DIR_NAME}/JoltPhysics/Build SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE Jolt)
set_target_properties(Jolt PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} Jolt)
if (NOT IS_RELEASE_BUILD)
    target_compile_definitions(Jolt PUBLIC JPH_DEBUG_RENDERER)    
endif()

# Add Angel Script.
message(STATUS "${PROJECT_NAME}: adding external dependency \"Angel Script\"...")
set(AS_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/angelscript/sdk/angelscript/projects/cmake ${DEPENDENCY_BUILD_DIR_NAME}/angelscript SYSTEM)
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE angelscript)
set_target_properties(angelscript PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} angelscript)
# Angel Script addons:
set(AS_ADDONS_TARGET angelscript_addons)
add_library(${AS_ADDONS_TARGET} STATIC
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptstdstring/scriptstdstring.cpp
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptstdstring/scriptstdstring.h
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptstdstring/scriptstdstring_utils.cpp

    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptbuilder/scriptbuilder.cpp
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptbuilder/scriptbuilder.h

    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scripthelper/scripthelper.cpp
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scripthelper/scripthelper.h

    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptmath/scriptmath.cpp
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptmath/scriptmath.h
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptmath/scriptmathcomplex.cpp
    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/scriptmath/scriptmathcomplex.h

    ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on/autowrapper/aswrappedcall.h
)
target_include_directories(${AS_ADDONS_TARGET} PUBLIC ${RELATIVE_EXT_PATH}/angelscript/sdk/add_on)
target_compile_definitions(${AS_ADDONS_TARGET} PRIVATE AS_USE_STLNAMES=1)
target_compile_definitions(${AS_ADDONS_TARGET} PRIVATE AS_USE_FLOAT=1)
target_link_libraries(${AS_ADDONS_TARGET} PUBLIC angelscript)
set_target_properties(${AS_ADDONS_TARGET} PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${AS_ADDONS_TARGET} angelscript)
# Link addons:
target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE ${AS_ADDONS_TARGET})
add_dependencies(${ENGINE_LIB_DEPS_TARGET} ${AS_ADDONS_TARGET})

if (NOT IS_RELEASE_BUILD)
    # Add tracy.
    message(STATUS "${PROJECT_NAME}: adding external dependency \"tracy\"...")
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
    add_subdirectory(${RELATIVE_EXT_PATH}/tracy ${DEPENDENCY_BUILD_DIR_NAME}/tracy SYSTEM)
    target_link_libraries(${ENGINE_LIB_DEPS_TARGET} INTERFACE TracyClient)
    set_target_properties(TracyClient PROPERTIES FOLDER ${EXTERNAL_FOLDER})
    add_dependencies(${ENGINE_LIB_DEPS_TARGET} TracyClient)

    add_compile_definitions(ENGINE_PROFILER_ENABLED)
endif()

# Link engine library to dependencies.
target_link_libraries(${ENGINE_LIB_TARGET} PUBLIC ${ENGINE_LIB_DEPS_TARGET})
target_link_libraries(${ENGINE_LIB_TARGET_FOR_EDITOR} PUBLIC ${ENGINE_LIB_DEPS_TARGET})
