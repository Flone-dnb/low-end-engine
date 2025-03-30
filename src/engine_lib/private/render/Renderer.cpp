#include "render/Renderer.h"

// Custom.
#include "game/Window.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "game/node/MeshNode.h"
#include "render/shader/LightSourceShaderArray.h"
#include "render/font/FontManager.h"
#include "misc/ProjectPaths.h"
#include "render/LightSourceManager.h"
#include "render/UiManager.h"

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

std::variant<std::unique_ptr<Renderer>, Error> Renderer::create(Window* pWindow) {
    // Create context.
    const auto pContext = SDL_GL_CreateContext(pWindow->getSdlWindow());
    if (pContext == nullptr) [[unlikely]] {
        return Error(SDL_GetError());
    }

    // After creating the context - initialize GLAD.
    if (gladLoadGLES2Loader(SDL_GL_GetProcAddress) == 0) {
        return Error("failed to load OpenGL ES");
    }

    // Enable back face culling.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // front because we use DirectX/Vulkan conversion for everything

    // Enable depth testing.
    GL_CHECK_ERROR(glEnable(GL_DEPTH_TEST));
    glDepthFunc(GL_LESS);
    glClearDepthf(1.0F);

    // Setup clear color.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    // Disable VSync.
    if (SDL_GL_SetSwapInterval(0) < 0) [[unlikely]] {
        Error::showErrorAndThrowException(SDL_GetError());
    }

    // Set viewport.
    const auto windowSize = pWindow->getWindowSize();
    GL_CHECK_ERROR(glViewport(0, 0, static_cast<int>(windowSize.first), static_cast<int>(windowSize.second)));

    auto pRenderer = std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));

    // Set FPS limit.
    const auto iScreenRefreshRate = pWindow->getScreenRefreshRate();
    Logger::get().info(std::format("setting FPS limit to {} (screen refresh rate)", iScreenRefreshRate));
    pRenderer->setFpsLimit(iScreenRefreshRate);

    return pRenderer;
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;

    pLightSourceManager = std::unique_ptr<LightSourceManager>(new LightSourceManager(this));
    pUiManager = std::unique_ptr<UiManager>(new UiManager(this));
    pFontManager = FontManager::create(
        this,
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "RedHatDisplay-Light.ttf");

    // Initialize fences.
    for (auto& fence : frameSync.vFences) {
        fence = GL_CHECK_ERROR(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
    }
}

void Renderer::drawNextFrame() {
    // Make sure there was no GL error during the last frame.
    const auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an OpenGL error occurred during the last frame, error code: {}", lastError));
    }

    // Wait for GPU.
    glWaitSync(frameSync.vFences[frameSync.iCurrentFrameIndex], 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(frameSync.vFences[frameSync.iCurrentFrameIndex]);

    // Render to window framebuffer.
    GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    const auto pCameraManager = pWindow->getGameManager()->getCameraManager();

    {
        // Get camera and shader programs.
        auto& mtxShaderPrograms = shaderManager.getShaderPrograms();
        auto& mtxActiveCamera = pCameraManager->getActiveCamera();
        std::scoped_lock guardProgramsCamera(mtxShaderPrograms.first, mtxActiveCamera.first);

        // Draw mesh nodes for each shader program.
        for (auto& [sProgramName, shaderProgram] :
             mtxShaderPrograms.second[static_cast<size_t>(ShaderProgramUsage::MESH_NODE)]) {
            const auto& [pWeakPtr, pShaderProgram] = shaderProgram;

            // Set shader program.
            glUseProgram(pShaderProgram->getShaderProgramId());

            // Set camera uniforms.
            mtxActiveCamera.second->getCameraProperties()->getShaderConstantsSetter().setConstantsToShader(
                pShaderProgram);

            // Set light arrays.
            pLightSourceManager->setArrayPropertiesToShader(pShaderProgram);

            // Get mesh nodes.
            auto& mtxMeshNodes = pShaderProgram->getMeshNodesUsingThisProgram();
            std::scoped_lock guard(mtxMeshNodes.first);

            for (const auto& pMeshNode : mtxMeshNodes.second) {
                // Set mesh.
                auto& vao = pMeshNode->getVertexArrayObjectWhileSpawned();
                glBindVertexArray(vao.getVertexArrayObjectId());

                // Set mesh uniforms.
                pMeshNode->getShaderConstantsSetterWhileSpawned().setConstantsToShader(pShaderProgram);

                // Draw.
                glDrawElements(GL_TRIANGLES, vao.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);
            }
        }

        // Draw UI.
        pUiManager->renderUi();
    }

    SDL_GL_SwapWindow(pWindow->getSdlWindow());

    // Insert a fence.
    frameSync.vFences[frameSync.iCurrentFrameIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    frameSync.iCurrentFrameIndex = (frameSync.iCurrentFrameIndex + 1) % frameSync.vFences.size();

    calculateFrameStatistics();
}

#if defined(WIN32)
void Renderer::nanosleep(long long iNanoseconds) {
    iNanoseconds /= 100; // NOLINT: The time after which the state of the timer is to be set to signaled,
                         // in 100 nanosecond intervals.

    // Prepare some variables to use.
    HANDLE pTimer;
    LARGE_INTEGER interval;

    // Create timer.
    pTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (pTimer == NULL) [[unlikely]] {
        Logger::get().error(
            std::format(
                "failed to wait create a waitable timer for {} nanoseconds (error code: {})",
                iNanoseconds,
                GetLastError()));
    }

    // Set timer.
    interval.QuadPart = -iNanoseconds;
    if (SetWaitableTimer(pTimer, &interval, 0, NULL, NULL, FALSE) == 0) [[unlikely]] {
        CloseHandle(pTimer);
        Logger::get().error(
            std::format(
                "failed to set a waitable timer for {} nanoseconds (error code: {})",
                iNanoseconds,
                GetLastError()));
    }

    // Wait for it to be signaled.
    WaitForSingleObject(pTimer, INFINITE);

    // Delete timer.
    CloseHandle(pTimer);
}
#endif

void Renderer::calculateFrameStatistics() {
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

    // Update FPS limit stats.
    {
        // Check if FPS limit is set.
        if (renderStats.fpsLimitInfo.optionalTargetTimeToRenderFrameInNs.has_value()) {
            // Get time that we should keep.
            const auto targetTimeToRenderFrameInNs =
                *renderStats.fpsLimitInfo.optionalTargetTimeToRenderFrameInNs;

            // Get time spent on last frame.
            const auto frameTimeInNs = duration<double, nanoseconds::period>(
                                           steady_clock::now() - renderStats.fpsLimitInfo.frameStartTime)
                                           .count();

            // Check if the last frame was rendered too fast.
            if (targetTimeToRenderFrameInNs > frameTimeInNs) {
                // Calculate time to wait.
                const auto timeToWaitInNs = targetTimeToRenderFrameInNs - frameTimeInNs;

#if defined(WIN32)
                const auto adjustTime =
                    static_cast<double>(renderStats.fpsLimitInfo.iFpsLimit) * 0.0002;   // NOLINT: don't ask
                const auto timeToWaitFraction = std::clamp(1.0 - adjustTime, 0.9, 1.0); // NOLINT: trust me XD
                timeBeginPeriod(1);
                nanosleep(static_cast<long long>(std::floor(timeToWaitInNs * timeToWaitFraction)));
                timeEndPeriod(1);
#else
                struct timespec tim;
                struct timespec tim2;
                tim.tv_sec = 0;
                tim.tv_nsec = static_cast<long>(timeToWaitInNs);
                nanosleep(&tim, &tim2);
#endif
            }
        }

        // Update frame start/end time.
        renderStats.fpsLimitInfo.frameStartTime = steady_clock::now();
    }
}

Renderer::~Renderer() {
    for (auto& fence : frameSync.vFences) {
        glDeleteSync(fence);
    }

    SDL_GL_DeleteContext(pContext);
}

void Renderer::waitForGpuToFinishWorkUpToThisPoint() { glFinish(); }

void Renderer::setFpsLimit(unsigned int iNewFpsLimit) {
    // Update time to render a frame.
    if (iNewFpsLimit == 0) {
        renderStats.fpsLimitInfo.optionalTargetTimeToRenderFrameInNs = {};
        renderStats.fpsLimitInfo.iFpsLimit = 0;
    } else {
        renderStats.fpsLimitInfo.optionalTargetTimeToRenderFrameInNs =
            1000000000.0 / static_cast<double>(iNewFpsLimit); // NOLINT
        renderStats.fpsLimitInfo.iFpsLimit = iNewFpsLimit;
    }
}

unsigned int Renderer::getFpsLimit() { return renderStats.fpsLimitInfo.iFpsLimit; }

Window* Renderer::getWindow() const { return pWindow; }

GpuResourceManager& Renderer::getGpuResourceManager() { return gpuResourceManager; }

ShaderManager& Renderer::getShaderManager() { return shaderManager; }

LightSourceManager& Renderer::getLightSourceManager() { return *pLightSourceManager.get(); }

UiManager& Renderer::getUiManager() { return *pUiManager.get(); }

FontManager& Renderer::getFontManager() { return *pFontManager.get(); }

RenderStatistics& Renderer::getRenderStatistics() { return renderStats; }
