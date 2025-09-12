#include "render/Renderer.h"

// Custom.
#include "game/Window.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/geometry/ScreenQuadGeometry.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "game/node/MeshNode.h"
#include "render/FontManager.h"
#include "render/UiNodeManager.h"
#include "render/MeshNodeManager.h"
#include "render/GpuResourceManager.h"
#include "render/PostProcessManager.h"
#include "misc/ProjectPaths.h"
#include "material/TextureManager.h"
#include "io/ConfigManager.h"
#include "render/DebugDrawer.h"

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
    glCullFace(GL_BACK);

    // Use DirectX/Vulkan winding order.
    glFrontFace(GL_CW);

    // Setup clear values.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClearDepthf(1.0F);

    // Disable VSync.
    if (SDL_GL_SetSwapInterval(0) < 0) [[unlikely]] {
        Error::showErrorAndThrowException(SDL_GetError());
    }

    auto pRenderer = std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));

    // Set FPS limit.
    const auto iScreenRefreshRate = Window::getScreenRefreshRate();
    Logger::get().info(std::format("setting FPS limit to {} (screen refresh rate)", iScreenRefreshRate));
    pRenderer->setFpsLimit(iScreenRefreshRate);

    return pRenderer;
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;

    pShaderManager = std::unique_ptr<ShaderManager>(new ShaderManager(this));
    pTextureManager = std::unique_ptr<TextureManager>(new TextureManager());
    pFontManager = FontManager::create(this);

    pFullscreenQuad = GpuResourceManager::createQuad(false);

    // Initialize fences.
    for (auto& fence : frameSync.vFences) {
        fence = GL_CHECK_ERROR(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
    }

    recreateFramebuffers();
}

void Renderer::recreateFramebuffers() {
    const auto pGameManager = pWindow->getGameManager();

    // Update framebuffers (main, post-process).
    if (pGameManager != nullptr) {
        auto& mtxWorldData = pGameManager->getWorlds();
        std::scoped_lock guard(mtxWorldData.first);
        for (const auto& pWorld : mtxWorldData.second.vWorlds) {
            pWorld->getCameraManager().onWindowSizeChanged(pWindow);
        }
    }

    pFontManager->onWindowSizeChanged();

    // Notify UI manager.
    if (pGameManager != nullptr) {
        auto& mtxWorldData = pGameManager->getWorlds();
        std::scoped_lock guard(mtxWorldData.first);
        for (const auto& pWorld : mtxWorldData.second.vWorlds) {
            pWorld->getUiNodeManager().onWindowSizeChanged();
        }
    }
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
    glWaitSync(frameSync.vFences[frameSync.iCurrentFrameIndex], 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(frameSync.vFences[frameSync.iCurrentFrameIndex]);

    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();

#if defined(ENGINE_UI_ONLY)
    auto& mtxWorlds = pWindow->getGameManager()->getWorlds();
    std::scoped_lock guardWorlds(mtxWorlds.first);

    if (!mtxWorlds.second.vWorlds.empty()) {
        const auto pWorld = mtxWorlds.second.vWorlds[0].get();

        // Check camera.
        auto& mtxActiveCamera = pWorld->getCameraManager().getActiveCamera();
        std::scoped_lock cameraGuard(mtxActiveCamera.first);
        if (mtxActiveCamera.second.pNode != nullptr) {
            const auto& pMainFramebuffer = pWorld->getCameraManager().pMainFramebuffer;

            glBindFramebuffer(GL_FRAMEBUFFER, pMainFramebuffer->getFramebufferId());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, iWindowWidth, iWindowHeight);

            pWorld->getUiNodeManager().drawUiOnFramebuffer(pMainFramebuffer->getFramebufferId());

            copyFramebufferToWindowFramebuffer(
                *pMainFramebuffer, glm::ivec4(0, 0, iWindowWidth, iWindowHeight));
        }
    }
#else
    // Get active cameras from all worlds.
    auto& mtxWorlds = pWindow->getGameManager()->getWorlds();
    std::scoped_lock guardWorlds(mtxWorlds.first);

    struct WorldRenderInfo {
        World* pWorld = nullptr;
        CameraNode* pCameraNode = nullptr;
        glm::ivec4 viewportSize;
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

        pCameraProperties->setRenderTargetProportions(iViewportWidth, iViewportHeight);

        const auto iViewportX = static_cast<int>(static_cast<float>(iWindowWidth) * viewportRect.x);
        const auto iViewportLeftBottom = static_cast<int>(
            static_cast<float>(iWindowHeight) * (1.0F - std::min(1.0F, viewportRect.y + viewportRect.w)));

        vActiveCameras.push_back(
            {&mtxActiveCamera.first,
             WorldRenderInfo{
                 .pWorld = pWorld.get(),
                 .pCameraNode = mtxActiveCamera.second.pNode,
                 .viewportSize =
                     glm::ivec4(iViewportX, iViewportLeftBottom, iViewportWidth, iViewportHeight)}});
    }

    const auto pGameInstance = getWindow()->getGameManager()->getGameInstance();

    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Draw meshes from each camera.
        for (const auto& mtxActiveCamera : vActiveCameras) {
            const auto pWorld = mtxActiveCamera.second.pWorld;
            const auto pCameraProperties = mtxActiveCamera.second.pCameraNode->getCameraProperties();
            const auto& viewportSize = mtxActiveCamera.second.viewportSize;
            const auto& pFramebuffer = pWorld->getCameraManager().pMainFramebuffer;

            GL_CHECK_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, pFramebuffer->getFramebufferId()));
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glViewport(viewportSize.x, viewportSize.y, viewportSize.z, viewportSize.w);

            pWorld->getMeshNodeManager().drawMeshes(pCameraProperties, pWorld->getLightSourceManager());

            pGameInstance->onFinishedSubmittingMeshDrawCommands(
                mtxActiveCamera.second.pCameraNode, *pFramebuffer);
        }

        // Draw post processing before UI.
        for (const auto& mtxActiveCamera : vActiveCameras) {
            const auto pWorld = mtxActiveCamera.second.pWorld;
            const auto pCameraProperties = mtxActiveCamera.second.pCameraNode->getCameraProperties();
            const auto& viewportSize = mtxActiveCamera.second.viewportSize;

            // Use window-size viewport to avoid "applying viewport twice".
            glViewport(0, 0, iWindowWidth, iWindowHeight);
            {
                pWorld->getCameraManager().getPostProcessManager().drawPostProcessing(
                    *pFullscreenQuad, *pWorld->getCameraManager().pMainFramebuffer, pCameraProperties);
            }
            glViewport(viewportSize.x, viewportSize.y, viewportSize.z, viewportSize.w);
        }

        // Draw UI nodes on post-processing framebuffer.
        for (const auto& mtxActiveCamera : vActiveCameras) {
            const auto pWorld = mtxActiveCamera.second.pWorld;
            auto& postProcessManager = pWorld->getCameraManager().getPostProcessManager();
            const auto& viewportSize = mtxActiveCamera.second.viewportSize;

            glViewport(viewportSize.x, viewportSize.y, viewportSize.z, viewportSize.w);

            pWorld->getUiNodeManager().drawUiOnFramebuffer(
                postProcessManager.pFramebuffer->getFramebufferId());
        }

        // Copy results from different worlds in order (consider viewport size).
        for (const auto& mtxActiveCamera : vActiveCameras) {
            const auto pWorld = mtxActiveCamera.second.pWorld;
            auto& postProcessManager = pWorld->getCameraManager().getPostProcessManager();
            const auto& viewportSize = mtxActiveCamera.second.viewportSize;

            copyFramebufferToWindowFramebuffer(*postProcessManager.pFramebuffer, viewportSize);
        }

#if defined(DEBUG)
        // Draw debug after all worlds.
        auto pCameraProperties = vActiveCameras[0].second.pCameraNode->getCameraProperties();
#ifdef ENGINE_EDITOR
        if (vActiveCameras.size() > 1) {
            // Find game world.
            for (const auto& cameraInfo : vActiveCameras) {
                if (cameraInfo.second.pWorld->getName() == "game") {
                    pCameraProperties = cameraInfo.second.pCameraNode->getCameraProperties();
                    break;
                }
            }
        }
#endif
        DebugDrawer::get().drawDebugObjects(this, pCameraProperties, timeSincePrevCallInSec);
#endif
    }

    // Unlock cameras.
    for (auto& mtxActiveCamera : vActiveCameras) {
        mtxActiveCamera.first->unlock();
    }
#endif

    // Swap.
    SDL_GL_SwapWindow(pWindow->getSdlWindow());

    // Insert a fence.
    frameSync.vFences[frameSync.iCurrentFrameIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    frameSync.iCurrentFrameIndex =
        static_cast<unsigned int>((frameSync.iCurrentFrameIndex + 1) % frameSync.vFences.size());

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

void Renderer::copyFramebufferToWindowFramebuffer(
    Framebuffer& srcFramebuffer, const glm::ivec4& viewportSize) {
    // Prepare rect bounds (start/end pos) for blit.
    glm::ivec4 bounds = viewportSize;
    bounds.z += bounds.x;
    bounds.w += bounds.y;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer.getFramebufferId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(
        bounds.x, // src
        bounds.y,
        bounds.z,
        bounds.w,
        bounds.x, // dst
        bounds.y,
        bounds.z,
        bounds.w,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR);
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
                    1000 / limitInfo.iTimeStampsPerSecond, // NOLINT
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

Renderer::~Renderer() {
#if defined(DEBUG)
    DebugDrawer::get().destroy(); // clear render resources
#endif

    pFullscreenQuad = nullptr;
    pFontManager = nullptr;

    for (auto& fence : frameSync.vFences) {
        glDeleteSync(fence);
    }

    SDL_GL_DeleteContext(pContext);
}

void Renderer::setFpsLimit(unsigned int iNewFpsLimit) {
    auto& data = renderStats.fpsLimitInfo;

    // Update time to render a frame.
    if (iNewFpsLimit == 0) {
        data.optionalTargetTimeToRenderFrameInNs = {};
        data.iFpsLimit = 0;
    } else {
        data.optionalTargetTimeToRenderFrameInNs = 1000000000.0 / static_cast<double>(iNewFpsLimit); // NOLINT
        data.iFpsLimit = iNewFpsLimit;

#if defined(WIN32)
        data.iMinTimeStampsPerSecond = data.iTimeStampsPerSecond / iNewFpsLimit;
#endif
    }
}

ShaderManager& Renderer::getShaderManager() { return *pShaderManager; }

FontManager& Renderer::getFontManager() { return *pFontManager; }

TextureManager& Renderer::getTextureManager() { return *pTextureManager; }

RenderStatistics& Renderer::getRenderStatistics() { return renderStats; }
