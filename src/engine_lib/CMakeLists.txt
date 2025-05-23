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
    private/render/UiManager.cpp
    private/render/UiManager.h
    public/render/ShaderAlignmentConstants.hpp
    private/render/FontManager.cpp
    public/render/FontManager.h
    public/render/PostProcessManager.h
    public/render/UiLayer.hpp
    private/render/PostProcessManager.cpp
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
    private/game/geometry/MeshGeometry.cpp
    public/game/geometry/MeshGeometry.h
    public/game/geometry/PrimitiveMeshGenerator.h
    private/game/geometry/PrimitiveMeshGenerator.cpp
    private/game/geometry/ScreenQuadGeometry.cpp
    public/game/geometry/ScreenQuadGeometry.h
    public/math/GLMath.hpp
    public/math/MathHelpers.hpp
    public/io/Logger.h
    private/io/Logger.cpp
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
    private/game/World.h
    private/game/node/Node.cpp
    public/game/node/Node.h
    private/game/node/SpatialNode.cpp
    public/game/node/SpatialNode.h
    public/game/node/NodeTickGroup.hpp
    public/game/node/CameraNode.h
    private/game/node/CameraNode.cpp
    public/game/node/MeshNode.h
    private/game/node/MeshNode.cpp
    public/game/node/Sound2dNode.h
    private/game/node/Sound2dNode.cpp
    public/game/node/Sound3dNode.h
    private/game/node/Sound3dNode.cpp
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
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE) # we don't use SDL audio
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${RELATIVE_EXT_PATH}/SDL/include)
add_subdirectory(${RELATIVE_EXT_PATH}/SDL ${DEPENDENCY_BUILD_DIR_NAME}/SDL EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL2::SDL2)
set_target_properties(SDL2-static PROPERTIES FOLDER ${EXTERNAL_FOLDER})

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

# Add nameof.
target_include_directories(${PROJECT_NAME} PUBLIC ${RELATIVE_EXT_PATH}/nameof/include)

# Add FreeType.
message(STATUS "${PROJECT_NAME}: adding external dependency \"FreeType\"...")
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/freetype ${DEPENDENCY_BUILD_DIR_NAME}/freetype SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC freetype)
set_target_properties(freetype PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add hwinfo.
message(STATUS "${PROJECT_NAME}: adding external dependency \"hwinfo\"...")
set(HWINFO_SHARED OFF CACHE BOOL "" FORCE)
set(HWINFO_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/hwinfo ${DEPENDENCY_BUILD_DIR_NAME}/hwinfo SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC lfreist-hwinfo::hwinfo)
set_target_properties(hwinfo PROPERTIES FOLDER ${EXTERNAL_FOLDER})

# Add tinygltf.
message(STATUS "${PROJECT_NAME}: adding external dependency \"tinygltf\"...")
set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/tinygltf ${DEPENDENCY_BUILD_DIR_NAME}/tinygltf SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC tinygltf)
set_target_properties(tinygltf PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${PROJECT_NAME} tinygltf)

# Add fpng.
message(STATUS "${PROJECT_NAME}: adding external dependency \"fpng\"...")
add_library(fpng
    ${RELATIVE_EXT_PATH}/fpng/src/fpng.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/lodepng.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/pvpngreader.cpp
    ${RELATIVE_EXT_PATH}/fpng/src/fpng.h
)
if (NOT MSVC)
    target_compile_options(fpng PRIVATE -msse4.1 -mpclmul)
    target_compile_definitions(fpng PRIVATE FPNG_NO_SSE=0)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ${RELATIVE_EXT_PATH}/fpng/src/)
target_link_libraries(${PROJECT_NAME} PUBLIC fpng)
set_target_properties(fpng PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${PROJECT_NAME} fpng)

# Add SFML.
message(STATUS "${PROJECT_NAME}: adding external dependency \"SFML\"...")
set(SFML_BUILD_WINDOW OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_STATIC_LIBRARIES ON CACHE BOOL "" FORCE)
add_subdirectory(${RELATIVE_EXT_PATH}/SFML ${DEPENDENCY_BUILD_DIR_NAME}/SFML SYSTEM)
target_link_libraries(${PROJECT_NAME} PUBLIC sfml-audio)
set_target_properties(sfml-audio PROPERTIES FOLDER ${EXTERNAL_FOLDER})
add_dependencies(${PROJECT_NAME} sfml-audio)

if (NOT IS_RELEASE_BUILD)
    # Add tracy.
    message(STATUS "${PROJECT_NAME}: adding external dependency \"tracy\"...")
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
    add_subdirectory(${RELATIVE_EXT_PATH}/tracy ${DEPENDENCY_BUILD_DIR_NAME}/tracy SYSTEM)
    target_link_libraries(${PROJECT_NAME} PUBLIC TracyClient)
    set_target_properties(TracyClient PROPERTIES FOLDER ${EXTERNAL_FOLDER})
    add_dependencies(${PROJECT_NAME} TracyClient)

    add_compile_definitions(ENGINE_PROFILER_ENABLED)
endif()
