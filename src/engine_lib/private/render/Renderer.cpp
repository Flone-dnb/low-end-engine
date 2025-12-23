#include "render/Renderer.h"

// Custom.
#include "game/Window.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "render/FontManager.h"
#include "render/UiNodeManager.h"
#include "render/MeshRenderer.h"
#include "render/GpuResourceManager.h"
#include "material/TextureManager.h"
#include "render/DebugDrawer.h"
#include "render/ShaderManager.h"
#include "game/DebugConsole.h"
#include "material/TextureHandle.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/wrapper/VertexArrayObject.h"
#include "render/GpuTimeQuery.hpp"
#include "render/GpuDebugMarker.hpp"
#include "render/ParticleRenderer.h"

// External.
#include "glad/glad.h"
#if defined(WIN32)
#define NOMINMAX
#include <Windows.h>
#pragma comment(lib, "Winmm.lib")
#elif __linux__
#include <unistd.h>
#include <time.h>
#endif

#if defined(DEBUG)
void GLAPIENTRY debugMessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam) {
    if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
        // We display shader compilation errors using ShaderManager.
        return;
    }

    if (type != GL_DEBUG_TYPE_ERROR) {
        return;
    }

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        Error::showErrorAndThrowException(std::format("GL debug message: {}", message));
    }
}
#endif

void DistanceFogSettings::setFogRange(const glm::vec2& range) {
    fogRange.x = std::max(range.x, 0.0f);
    fogRange.y = std::max(range.y, fogRange.x);
}

void DistanceFogSettings::setColor(const glm::vec3& color) { this->color = color; }

void DistanceFogSettings::setFogHeightOnSky(float fogHeight) {
    fogHeightOnSky = std::clamp(fogHeight, 0.0f, 1.0f);
}

SkyboxSettings::SkyboxSettings() {}
SkyboxSettings::~SkyboxSettings() {}
Renderer::SkyboxData::SkyboxData() {}
Renderer::SkyboxData::~SkyboxData() {}

std::unique_ptr<Renderer> Renderer::create(Window* pWindow) {
    // Create context.
    const auto pContext = SDL_GL_CreateContext(pWindow->getSdlWindow());
    if (pContext == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(SDL_GetError());
    }

    // After creating the context - initialize GLAD.
    if (gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) == 0) [[unlikely]] {
        Error::showErrorAndThrowException("failed to load OpenGL ES");
    }

#if defined(ENGINE_DEBUG_TOOLS)
    if (GLAD_GL_EXT_disjoint_timer_query != 1) {
        Error::showErrorAndThrowException("the GPU does not support OpenGL extension "
                                          "GL_EXT_disjoint_timer_query which is required for debug tools");
    }
#endif

#if defined(DEBUG)
    if (GLAD_GL_KHR_debug != 1) {
        Error::showErrorAndThrowException(
            "the GPU does not support GL_KHR_DEBUG extension which is required for debug builds");
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugMessageCallback, nullptr);

    // Enable all error messages
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    // Enable back face culling.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Setup clear values.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepthf(1.0f);

    // Disable VSync.
    if (!SDL_GL_SetSwapInterval(0)) [[unlikely]] {
        Error::showErrorAndThrowException(SDL_GetError());
    }

    auto pRenderer = std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));

    // Set FPS limit.
    const auto iScreenRefreshRate = Window::getScreenRefreshRate();
    Log::info(std::format("setting FPS limit to {} (screen refresh rate)", iScreenRefreshRate));
    pRenderer->setFpsLimit(iScreenRefreshRate);

    return pRenderer;
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;

    iCurrentGlDepthFunc = GL_LEQUAL; // less/equal is also needed for main pass (after z prepass)
    glDepthFunc(iCurrentGlDepthFunc);

    pShaderManager = std::unique_ptr<ShaderManager>(new ShaderManager(this));
    pTextureManager = std::unique_ptr<TextureManager>(new TextureManager());
    pFontManager = FontManager::create(this);

    pFullscreenQuad = GpuResourceManager::createScreenQuad();

    const std::vector<glm::vec3> vSkyboxVertices = {
        glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec3(-1.0f, 1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 1.0f, -1.0f),
        glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec3(-1.0f, -1.0f, 1.0f),

        glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec3(1.0f, -1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec3(1.0f, -1.0f, 1.0f),   glm::vec3(-1.0f, -1.0f, 1.0f),

        glm::vec3(-1.0f, 1.0f, -1.0f),  glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec3(-1.0f, 1.0f, 1.0f),   glm::vec3(-1.0f, 1.0f, -1.0f),

        glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(1.0f, -1.0f, -1.0f),
        glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(1.0f, -1.0f, 1.0f),
    };
    skyboxData.pCubeVao = GpuResourceManager::createVertexArrayObject(false, vSkyboxVertices);

    // Initialize fences.
    for (auto& fence : frameSyncData.vFences) {
        fence = GL_CHECK_ERROR(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
    }

    recreateFramebuffers();

#if defined(ENGINE_DEBUG_TOOLS)
    DebugConsole::registerCommand("setFpsLimit", [this](GameInstance*, int iNewLimit) {
        if (iNewLimit < 0) {
            iNewLimit = 0;
        }
        setFpsLimit(static_cast<unsigned int>(iNewLimit));
    });

    // Initialize queries.
    for (auto& frameQueries : frameSyncData.vFrameQueries) {
        GL_CHECK_ERROR(glGenQueriesEXT(1, &frameQueries.iGlQueryStartFrameTimestamp));
        GL_CHECK_ERROR(glGenQueriesEXT(1, &frameQueries.iGlQueryEndFrameTimestamp));

        glQueryCounterEXT(frameQueries.iGlQueryStartFrameTimestamp, GL_TIMESTAMP_EXT);
        glQueryCounterEXT(frameQueries.iGlQueryEndFrameTimestamp, GL_TIMESTAMP_EXT);

        GL_CHECK_ERROR(glGenQueriesEXT(1, &frameQueries.iGlQueryToDrawSkybox));
        {
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawSkybox);
        }

        GL_CHECK_ERROR(glGenQueriesEXT(1, &frameQueries.iGlQueryToDrawUi));
        {
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawUi);
        }

        GL_CHECK_ERROR(glGenQueriesEXT(1, &frameQueries.iGlQueryToDrawDebug));
        {
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawDebug);
        }
    }
#endif
}

Renderer::~Renderer() {
#if defined(ENGINE_DEBUG_TOOLS)
    DebugDrawer::get().destroy(); // clear render resources
    for (auto& frameQueries : frameSyncData.vFrameQueries) {
        GL_CHECK_ERROR(glDeleteQueriesEXT(1, &frameQueries.iGlQueryStartFrameTimestamp));
        GL_CHECK_ERROR(glDeleteQueriesEXT(1, &frameQueries.iGlQueryEndFrameTimestamp));
        GL_CHECK_ERROR(glDeleteQueriesEXT(1, &frameQueries.iGlQueryToDrawSkybox));
        GL_CHECK_ERROR(glDeleteQueriesEXT(1, &frameQueries.iGlQueryToDrawUi));
        GL_CHECK_ERROR(glDeleteQueriesEXT(1, &frameQueries.iGlQueryToDrawDebug));
    }
#endif

    skyboxData.pCubeVao = nullptr;
    skyboxData.pShaderProgram = nullptr;
    pFullscreenQuad = nullptr;
    pFontManager = nullptr;
    pTextureManager = nullptr;
    pShaderManager = nullptr; // delete shaders before context

    for (auto& fence : frameSyncData.vFences) {
        glDeleteSync(fence);
    }

    SDL_GL_DestroyContext(pContext);
}

void Renderer::recreateFramebuffers() {
    // Don't need to re-create any renderer framebuffers because we use window's framebuffer.
    pFontManager->onWindowSizeChanged();
}

void Renderer::onWindowSizeChanged() { recreateFramebuffers(); }

void Renderer::drawNextFrame(float timeSincePrevCallInSec) {
    PROFILE_FUNC

    // Make sure there was no GL error during the last frame.
    auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an OpenGL error occurred during the last frame, error code: {}", lastError));
    }

    // Wait for GPU.
    const auto waitResult =
        glClientWaitSync(frameSyncData.vFences[frameSyncData.iCurrentFrameIndex], 0, GL_TIMEOUT_IGNORED);
    if (waitResult == GL_WAIT_FAILED) [[unlikely]] {
        Error::showErrorAndThrowException("failed to wait for a GPU fence");
    }
    glDeleteSync(frameSyncData.vFences[frameSyncData.iCurrentFrameIndex]);
    auto& frameQueries = frameSyncData.vFrameQueries[frameSyncData.iCurrentFrameIndex];

    auto& mtxWorlds = pWindow->getGameManager()->getWorlds();
    std::scoped_lock guardWorlds(mtxWorlds.first);

#if defined(ENGINE_DEBUG_TOOLS)
    // Prepare a lambda to query time.
    const auto getQueryTimeMs = [](unsigned int iQuery) -> float {
        GLuint iAvailable = 0;
        GLuint64 iTimeElapsed = 0;

        glGetQueryObjectuivEXT(iQuery, GL_QUERY_RESULT_AVAILABLE_EXT, &iAvailable);
        if (iAvailable == GL_FALSE) {
            // We waited for a GPU fence and all previous operations should be finished at this point
            // but this situation still may rarely happen.
            return -1.0f;
        }

        glGetQueryObjectui64vEXT(iQuery, GL_QUERY_RESULT_EXT, &iTimeElapsed);
        return static_cast<float>(iTimeElapsed) / 1000000.0f; // nanoseconds to milliseconds
    };
    auto& stats = DebugConsole::getStats();
    {
        GLint64 iStartTime = 0;
        GLint64 iEndTime = 0;
        glGetQueryObjecti64vEXT(frameQueries.iGlQueryStartFrameTimestamp, GL_QUERY_RESULT, &iStartTime);
        glGetQueryObjecti64vEXT(frameQueries.iGlQueryEndFrameTimestamp, GL_QUERY_RESULT, &iEndTime);
        stats.gpuTimeDrawFrameMs =
            static_cast<float>(iEndTime - iStartTime) / 1000000.0f; // nanoseconds to milliseconds
    }
    stats.gpuTimeDrawSkyboxMs = getQueryTimeMs(frameQueries.iGlQueryToDrawSkybox);
    stats.gpuTimeDrawUiMs = getQueryTimeMs(frameQueries.iGlQueryToDrawUi);
    stats.gpuTimeDrawDebug = getQueryTimeMs(frameQueries.iGlQueryToDrawDebug);
    stats.gpuTimeDrawShadowPassMs = 0.0f;
    stats.gpuTimeDrawDepthPrepassMs = 0.0f;
    stats.gpuTimeDrawMeshesMs = 0.0f;
    for (const auto& pWorld : mtxWorlds.second.vWorlds) {
        const auto& worldQueries = pWorld->getFrameQueries()[frameSyncData.iCurrentFrameIndex];
        stats.gpuTimeDrawShadowPassMs += getQueryTimeMs(worldQueries.iGlQueryToDrawShadowPass);
        stats.gpuTimeDrawDepthPrepassMs += getQueryTimeMs(worldQueries.iGlQueryToDrawDepthPrepass);
        stats.gpuTimeDrawMeshesMs += getQueryTimeMs(worldQueries.iGlQueryToDrawMeshes);
    }

    const auto cpuFrameStartCounter = SDL_GetPerformanceCounter();

    glQueryCounterEXT(frameQueries.iGlQueryStartFrameTimestamp, GL_TIMESTAMP_EXT);
#endif

    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();

#if defined(ENGINE_UI_ONLY)
    if (!mtxWorlds.second.vWorlds.empty()) {
        // Just render the first world.
        const auto pWorld = mtxWorlds.second.vWorlds[0].get();

        // Draw on window's framebuffer.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, iWindowWidth, iWindowHeight);

        pWorld->getUiNodeManager().drawUiOnActiveFramebuffer();
    }
#else
    struct WorldRenderInfo {
        World* pWorld = nullptr;
        CameraNode* pCameraNode = nullptr;
        glm::ivec4 viewportSize;
        glm::mat4 viewProjectionMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        unsigned int iGlQueryToDrawShadowPass = 0;
        unsigned int iGlQueryToDrawDepthPrepass = 0;
        unsigned int iGlQueryToDrawMeshes = 0;
    };
    std::vector<std::pair<std::recursive_mutex*, WorldRenderInfo>> vActiveCameras;
    vActiveCameras.reserve(2);

    for (const auto& pWorld : mtxWorlds.second.vWorlds) {
        // Check camera.
        auto& mtxActiveCamera = pWorld->getCameraManager().getActiveCamera();
        mtxActiveCamera.first.lock();
        if (mtxActiveCamera.second.pNode == nullptr) {
            mtxActiveCamera.first.unlock();
            continue;
        }
        const auto pCameraProperties = mtxActiveCamera.second.pNode->getCameraProperties();

        // Prepare viewport.
        const auto viewportRect = pCameraProperties->getViewport();
        const auto iViewportWidth =
            static_cast<unsigned int>(static_cast<float>(iWindowWidth) * viewportRect.z);
        const auto iViewportHeight =
            static_cast<unsigned int>(static_cast<float>(iWindowHeight) * viewportRect.w);

        const auto iViewportX = static_cast<int>(static_cast<float>(iWindowWidth) * viewportRect.x);
        const auto iViewportLeftBottom = static_cast<int>(
            static_cast<float>(iWindowHeight) * (1.0f - std::min(1.0f, viewportRect.y + viewportRect.w)));

        pCameraProperties->setRenderTargetProportions(iViewportWidth, iViewportHeight);
        const auto& vWorldQueries = pWorld->getFrameQueries()[frameSyncData.iCurrentFrameIndex];

        vActiveCameras.push_back(
            {&mtxActiveCamera.first,
             WorldRenderInfo{
                 .pWorld = pWorld.get(),
                 .pCameraNode = mtxActiveCamera.second.pNode,
                 .viewportSize = glm::ivec4(iViewportX, iViewportLeftBottom, iViewportWidth, iViewportHeight),
                 .viewProjectionMatrix =
                     pCameraProperties->getProjectionMatrix() * pCameraProperties->getViewMatrix(),
                 .viewMatrix = pCameraProperties->getViewMatrix(),
                 .projectionMatrix = pCameraProperties->getProjectionMatrix(),
                 .iGlQueryToDrawShadowPass = vWorldQueries.iGlQueryToDrawShadowPass,
                 .iGlQueryToDrawDepthPrepass = vWorldQueries.iGlQueryToDrawDepthPrepass,
                 .iGlQueryToDrawMeshes = vWorldQueries.iGlQueryToDrawMeshes}});
    }

    const auto pGameInstance = getWindow()->getGameManager()->getGameInstance();

    if (!vActiveCameras.empty()) {
        // Prepare game world info.
        WorldRenderInfo* pGameWorldRenderInfo = nullptr;
#if defined(ENGINE_EDITOR)
        // Find game world. In the editor debug drawing should only be done for game world.
        for (auto& cameraInfo : vActiveCameras) {
            if (cameraInfo.second.pWorld->getName() == "game") {
                pGameWorldRenderInfo = &cameraInfo.second;
                break;
            }
        }
#else
        // In the game just use the first camera.
        pGameWorldRenderInfo = &vActiveCameras[0].second;
#endif

        // Rendering to window's framebuffer.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(iCurrentGlDepthFunc);

        // Draw meshes.
        {
#if defined(ENGINE_DEBUG_TOOLS)
            auto& debugStats = DebugConsole::getStats();
            debugStats.cpuTimeToSubmitShadowPassMs = 0.0f;
            debugStats.cpuTimeToSubmitDepthPrepassMs = 0.0f;
            debugStats.cpuTimeToSubmitMeshesMs = 0.0f;
#endif
            for (const auto& mtxActiveCamera : vActiveCameras) {
                GPU_MARKER_SCOPED("draw meshes of a world");

                const auto pWorld = mtxActiveCamera.second.pWorld;
                const auto& viewportSize = mtxActiveCamera.second.viewportSize;

                // Get camera frustum.
                const auto& frustum = mtxActiveCamera.second.pCameraNode->getCameraProperties()
                                          ->getCameraFrustumMaybeOutdated();
                static_assert(
                    sizeof(WorldRenderInfo::viewProjectionMatrix) > 1,
                    "because we requested view and projection matrix the frustum here is up to date");

                const auto& renderData = mtxActiveCamera.second;

                pWorld->getMeshRenderer().drawMeshes(
                    this,
                    viewportSize,
                    renderData.viewMatrix,
                    renderData.viewProjectionMatrix,
                    frustum,
                    pWorld->getLightSourceManager(),
                    renderData.iGlQueryToDrawShadowPass,
                    renderData.iGlQueryToDrawDepthPrepass,
                    renderData.iGlQueryToDrawMeshes);
            }
        }

        // Draw particles.
        {
            for (const auto& mtxActiveCamera : vActiveCameras) {
                GPU_MARKER_SCOPED("draw particles of a world");

                const auto pWorld = mtxActiveCamera.second.pWorld;
                const auto& renderData = mtxActiveCamera.second;

                pWorld->getParticleRenderer().drawParticles(
                    renderData.viewMatrix, renderData.projectionMatrix);
            }
        }

        // Draw skybox.
        if (skyboxData.optSettings.has_value() && pGameWorldRenderInfo != nullptr) {
            if (skyboxData.pShaderProgram == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected skybox shader program to be valid");
            }

            GPU_MARKER_SCOPED("draw skybox");
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawSkybox);

            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE);

            glUseProgram(skyboxData.pShaderProgram->getShaderProgramId());
            glBindVertexArray(skyboxData.pCubeVao->getVertexArrayObjectId());

            // Cubemap uniform.
            const auto bIsSkyboxSet = skyboxData.optSettings->pOptSkyboxCubemap != nullptr;
            if (skyboxData.iIsSkyboxCubemapSetUniform != -1) {
                glUniform1i(skyboxData.iIsSkyboxCubemapSetUniform, static_cast<int>(bIsSkyboxSet));
            }
            if (bIsSkyboxSet) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxData.optSettings->pOptSkyboxCubemap->getTextureId());
            }

            // View projection matrix uniform.
            glUniformMatrix4fv(
                skyboxData.iViewProjectionMatrixUniform,
                1,
                GL_FALSE,
                glm::value_ptr(pGameWorldRenderInfo->viewProjectionMatrix));

            // Distance fog uniforms.
            glUniform1f(skyboxData.iFogHeightOnSkyUniform, -1.0f);
            if (optDistanceFogSettings.has_value()) {
                glUniform1f(skyboxData.iFogHeightOnSkyUniform, optDistanceFogSettings->getFogHeightOnSky());
                const auto color = optDistanceFogSettings->getColor();
                glUniform3fv(skyboxData.iFogColorUniform, 1, glm::value_ptr(color));
            }

            glDrawArrays(GL_TRIANGLES, 0, 36);

            glDepthMask(GL_TRUE);
            glDepthFunc(iCurrentGlDepthFunc);
        }

        // Draw UI.
        {
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawUi);
#if defined(ENGINE_DEBUG_TOOLS)
            const auto cpuSubmitUiStartCounter = SDL_GetPerformanceCounter();
#endif
            for (const auto& mtxActiveCamera : vActiveCameras) {
                GPU_MARKER_SCOPED("draw ui of a world");

                const auto pWorld = mtxActiveCamera.second.pWorld;
                const auto& viewportSize = mtxActiveCamera.second.viewportSize;

                glViewport(viewportSize.x, viewportSize.y, viewportSize.z, viewportSize.w);

                pWorld->getUiNodeManager().drawUiOnActiveFramebuffer();
            }
#if defined(ENGINE_DEBUG_TOOLS)
            DebugConsole::getStats().cpuTimeToSubmitUiMs =
                static_cast<float>(SDL_GetPerformanceCounter() - cpuSubmitUiStartCounter) * 1000.0f /
                static_cast<float>(SDL_GetPerformanceFrequency());
#endif
        }

#if defined(ENGINE_DEBUG_TOOLS)
        if (pGameWorldRenderInfo != nullptr) {
            // Draw debug after all worlds.
            const auto cpuSubmitDebugStartCounter = SDL_GetPerformanceCounter();

            GPU_MARKER_SCOPED("draw debug objects");
            MEASURE_GPU_TIME_SCOPED(frameQueries.iGlQueryToDrawDebug);

            DebugDrawer::get().drawDebugObjects(
                this, pGameWorldRenderInfo->viewProjectionMatrix, timeSincePrevCallInSec);

            DebugConsole::getStats().cpuTimeToSubmitDebugDrawMs =
                static_cast<float>(SDL_GetPerformanceCounter() - cpuSubmitDebugStartCounter) * 1000.0f /
                static_cast<float>(SDL_GetPerformanceFrequency());
        }
#endif
    }

    // Unlock cameras.
    for (auto& mtxActiveCamera : vActiveCameras) {
        mtxActiveCamera.first->unlock();
    }
#endif

#if defined(ENGINE_DEBUG_TOOLS)
    // Get CPU time before swap as it might block the thread until the GPU is finished.
    DebugConsole::getStats().cpuSubmitFrameTimeMs =
        static_cast<float>(SDL_GetPerformanceCounter() - cpuFrameStartCounter) * 1000.0f /
        static_cast<float>(SDL_GetPerformanceFrequency());
    glQueryCounterEXT(frameQueries.iGlQueryEndFrameTimestamp, GL_TIMESTAMP_EXT);

    const auto cpuSwapStartCounter = SDL_GetPerformanceCounter();
#endif

    SDL_GL_SwapWindow(pWindow->getSdlWindow());

#if defined(ENGINE_DEBUG_TOOLS)
    DebugConsole::getStats().cpuTimeFlipSwapchainMs =
        static_cast<float>(SDL_GetPerformanceCounter() - cpuSwapStartCounter) * 1000.0f /
        static_cast<float>(SDL_GetPerformanceFrequency());
#endif

    // Insert a fence.
    frameSyncData.vFences[frameSyncData.iCurrentFrameIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    frameSyncData.iCurrentFrameIndex =
        static_cast<unsigned int>((frameSyncData.iCurrentFrameIndex + 1) % frameSyncData.vFences.size());

    calculateFrameStatistics();

#if defined(ENGINE_PROFILER_ENABLED)
    FrameMark;
#endif

    // Make sure there was no GL error during frame submission.
    lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an OpenGL error occurred while submitting a new frame, error code: {}", lastError));
    }
}

void Renderer::calculateFrameStatistics() {
    PROFILE_FUNC

    using namespace std::chrono;

    // Update FPS stats.
    {
        // Get elapsed time.
        const auto iTimeSinceFpsUpdateInSec =
            duration_cast<seconds>(steady_clock::now() - renderStats.fpsInfo.timeAtLastFpsUpdate).count();

        // Count the new present call.
        renderStats.fpsInfo.iPresentCountSinceFpsUpdate += 1;

        // See if 1 second has passed.
        if (iTimeSinceFpsUpdateInSec >= 1) {
            // Save FPS.
            renderStats.fpsInfo.iFramesPerSecond = renderStats.fpsInfo.iPresentCountSinceFpsUpdate;

            // Reset present count.
            renderStats.fpsInfo.iPresentCountSinceFpsUpdate = 0;

            // Restart time.
            renderStats.fpsInfo.timeAtLastFpsUpdate = steady_clock::now();
        }
    }

    // FPS limit.
    if (renderStats.fpsLimitInfo.optionalTargetTimeToRenderFrameInNs.has_value()) {
        PROFILE_SCOPE("fps limit sleep")

        auto& limitInfo = renderStats.fpsLimitInfo;

#if defined(WIN32)
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        long long iFrameEndTime = counter.QuadPart;

        unsigned long iSleepTimeMs = 0;
        if (iFrameEndTime < limitInfo.iPerfCounterLastFrameEnd + limitInfo.iMinTimeStampsPerSecond) {
            iSleepTimeMs = static_cast<unsigned long>(std::max(
                (limitInfo.iPerfCounterLastFrameEnd + limitInfo.iMinTimeStampsPerSecond - iFrameEndTime) *
                    1000 / limitInfo.iTimeStampsPerSecond,
                1LL));

            if (iSleepTimeMs > 0) {
                timeBeginPeriod(1);
                Sleep(iSleepTimeMs);
                timeEndPeriod(1);
            }
        }
#else
        const auto targetTimeToRenderFrameInNs = *limitInfo.optionalTargetTimeToRenderFrameInNs;

        const auto frameTimeInNs =
            duration<double, nanoseconds::period>(steady_clock::now() - limitInfo.frameStartTime).count();

        if (targetTimeToRenderFrameInNs > frameTimeInNs) {
            // Calculate time to wait.
            const auto timeToWaitInNs = targetTimeToRenderFrameInNs - frameTimeInNs;

            struct timespec tim;
            struct timespec tim2;
            tim.tv_sec = 0;
            tim.tv_nsec = static_cast<long>(timeToWaitInNs);
            nanosleep(&tim, &tim2);
        }
#endif
    }

    // Update frame start/end time.
    renderStats.fpsLimitInfo.frameStartTime = steady_clock::now();

#if defined(WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    renderStats.fpsLimitInfo.iPerfCounterLastFrameEnd = counter.QuadPart;
#endif
}

void Renderer::setFpsLimit(unsigned int iNewFpsLimit) {
    auto& data = renderStats.fpsLimitInfo;

    // Update time to render a frame.
    if (iNewFpsLimit == 0) {
        data.optionalTargetTimeToRenderFrameInNs = {};
        data.iFpsLimit = 0;
    } else {
        data.optionalTargetTimeToRenderFrameInNs = 1000000000.0 / static_cast<double>(iNewFpsLimit);
        data.iFpsLimit = iNewFpsLimit;

#if defined(WIN32)
        data.iMinTimeStampsPerSecond = data.iTimeStampsPerSecond / iNewFpsLimit;
#endif
    }
}

void Renderer::setSkybox(std::optional<SkyboxSettings> newSkyboxSettings) {
    skyboxData.optSettings = std::move(*newSkyboxSettings);

    if (skyboxData.optSettings.has_value()) {
        skyboxData.pShaderProgram = pShaderManager->getShaderProgram(
            "engine/shaders/skybox/skybox.vert.glsl", skyboxData.optSettings->sRelativePathToFragmentShader);

        skyboxData.iViewProjectionMatrixUniform =
            skyboxData.pShaderProgram->getShaderUniformLocation("viewProjectionMatrix");
        skyboxData.iIsSkyboxCubemapSetUniform =
            skyboxData.pShaderProgram->tryGetShaderUniformLocation("bIsSkyboxCubemapSet");
        skyboxData.iFogColorUniform = skyboxData.pShaderProgram->getShaderUniformLocation("fogColor");
        skyboxData.iFogHeightOnSkyUniform =
            skyboxData.pShaderProgram->getShaderUniformLocation("fogHeightOnSky");
    }
}

ShaderManager& Renderer::getShaderManager() { return *pShaderManager; }

FontManager& Renderer::getFontManager() { return *pFontManager; }

TextureManager& Renderer::getTextureManager() { return *pTextureManager; }

RenderStatistics& Renderer::getRenderStatistics() { return renderStats; }
