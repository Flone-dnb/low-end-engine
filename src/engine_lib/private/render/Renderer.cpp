#include "render/Renderer.h"

// Custom.
#include "game/Window.h"
#include "render/wrapper/ShaderProgram.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "game/node/MeshNode.h"
#include "render/shader/LightSourceShaderArray.h"
#include "render/FontManager.h"
#include "misc/ProjectPaths.h"
#include "render/LightSourceManager.h"
#include "render/UiManager.h"
#include "material/TextureManager.h"
#include "render/GpuResourceManager.h"
#include "io/ConfigManager.h"

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

    // Set viewport.
    const auto windowSize = pWindow->getWindowSize();
    GL_CHECK_ERROR(glViewport(0, 0, static_cast<int>(windowSize.first), static_cast<int>(windowSize.second)));

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
    pLightSourceManager = std::unique_ptr<LightSourceManager>(new LightSourceManager(this));
    pUiManager = std::unique_ptr<UiManager>(new UiManager(this));
    pTextureManager = std::unique_ptr<TextureManager>(new TextureManager());
    pFontManager = FontManager::create(this);

    pFullscreenQuad = GpuResourceManager::createQuad(false);

    // Initialize fences.
    for (auto& fence : frameSync.vFences) {
        fence = GL_CHECK_ERROR(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0))
    }

    pGammaCorrectionShaderProgram = pShaderManager->getShaderProgram(
        "engine/shaders/postprocessing/PostProcessingQuad.vert.glsl",
        "engine/shaders/postprocessing/GammaCorrection.frag.glsl",
        ShaderProgramUsage::OTHER);

    // Read global config.
    const auto pathToConfig = ProjectPaths::getPathToBaseConfigDirectory() / sGlobalRenderConfigFilename;
    if (std::filesystem::exists(pathToConfig)) {
        ConfigManager config;
        const auto optionalError = config.loadFile(pathToConfig);
        if (optionalError.has_value()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to load render config file from \"{}\", error: {}",
                pathToConfig.string(),
                optionalError->getInitialMessage()));
        }
        gamma = std::clamp(config.getValue("", "gamma", gamma), 1.0F, 2.4F);

        bUserRenderConfigLoaded = true;
    }

    recreateFramebuffers();
}

void Renderer::recreateFramebuffers() {
    // Set viewport.
    const auto [iWindowWidth, iWindowHeight] = pWindow->getWindowSize();
    GL_CHECK_ERROR(glViewport(0, 0, static_cast<int>(iWindowWidth), static_cast<int>(iWindowHeight)));

#if !defined(ENGINE_UI_ONLY)
    if (pPostProcessManager == nullptr) {
        pPostProcessManager = std::unique_ptr<PostProcessManager>(
            new PostProcessManager(pShaderManager.get(), iWindowWidth, iWindowHeight));
    } else {
        pPostProcessManager->recreateFramebuffer(iWindowWidth, iWindowHeight);
    }
#endif

    // Main framebuffer.
    auto iDepthFormat = GL_DEPTH_COMPONENT24;
#if defined(ENGINE_UI_ONLY)
    iDepthFormat = 0;
#endif
    pMainFramebuffer =
        GpuResourceManager::createFramebuffer(iWindowWidth, iWindowHeight, GL_RGB8, iDepthFormat);

    pUiManager->onWindowSizeChanged();
    pFontManager->onWindowSizeChanged();
}

void Renderer::onWindowSizeChanged() { recreateFramebuffers(); }

void Renderer::drawNextFrame() {
    PROFILE_FUNC

    // Make sure there was no GL error during the last frame.
    const auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an OpenGL error occurred during the last frame, error code: {}", lastError));
    }

    // Wait for GPU.
    glWaitSync(frameSync.vFences[frameSync.iCurrentFrameIndex], 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(frameSync.vFences[frameSync.iCurrentFrameIndex]);

    const auto pCameraManager = pWindow->getGameManager()->getCameraManager();

    // Get camera and shader programs.
    auto& mtxShaderPrograms = pShaderManager->getShaderPrograms();
    auto& mtxActiveCamera = pCameraManager->getActiveCamera();
    std::scoped_lock guardProgramsCamera(mtxShaderPrograms.first, mtxActiveCamera.first);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

#if defined(ENGINE_UI_ONLY)
    if (pMainFramebuffer == nullptr) {
        // Gamme correction is not needed.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        pUiManager->drawUi(0);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, pMainFramebuffer->getFramebufferId());
        glClear(GL_COLOR_BUFFER_BIT);

        pUiManager->drawUi(pMainFramebuffer->getFramebufferId());

        drawGammaCorrectionScreenQuad(0, pMainFramebuffer->getColorTextureId());
    }
#else
    // Set framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, pMainFramebuffer->getFramebufferId());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (mtxActiveCamera.second != nullptr) {
        // Prepare a handy lambda.
        const auto drawMeshes =
            [&](const std::unordered_map<
                std::string,
                std::pair<std::weak_ptr<ShaderProgram>, ShaderProgram*>>& shaderPrograms) {
                for (auto& [sProgramName, shaderProgram] : shaderPrograms) {
                    const auto& [pWeakPtr, pShaderProgram] = shaderProgram;

                    PROFILE_SCOPE("render mesh nodes of shader program")
                    PROFILE_ADD_SCOPE_TEXT(sProgramName.c_str(), sProgramName.size());

                    // Set shader program.
                    glUseProgram(pShaderProgram->getShaderProgramId());

                    // Set camera uniforms.
                    mtxActiveCamera.second->getCameraProperties()
                        ->getShaderConstantsSetter()
                        .setConstantsToShader(pShaderProgram);

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
                        pMeshNode->getShaderConstantsSetterWhileSpawned().setConstantsToShader(
                            pShaderProgram);

                        // Draw.
                        glDrawElements(GL_TRIANGLES, vao.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);
                    }

                    // Clear texture slots (if they where used).
                    for (int i = GL_TEXTURE0; i < 4; i++) { // NOLINT
                        glActiveTexture(i);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }
                }
            };

        // Draw meshes.
        drawMeshes(mtxShaderPrograms.second[static_cast<size_t>(ShaderProgramUsage::MESH_NODE)]);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        {
            drawMeshes(
                mtxShaderPrograms.second[static_cast<size_t>(ShaderProgramUsage::TRANSPARENT_MESH_NODE)]);
        }
        glDisable(GL_BLEND);

        // Do post processing before UI.
        pPostProcessManager->drawPostProcessing(
            *pFullscreenQuad, *pMainFramebuffer, mtxActiveCamera.second->getCameraProperties());

        // Draw UI on top of post-processing results.
        pUiManager->drawUi(pPostProcessManager->pFramebuffer->getFramebufferId());

        drawGammaCorrectionScreenQuad(0, pPostProcessManager->pFramebuffer->getColorTextureId());
    }
#endif

    // Swap.
    SDL_GL_SwapWindow(pWindow->getSdlWindow());

    // Insert a fence.
    frameSync.vFences[frameSync.iCurrentFrameIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    frameSync.iCurrentFrameIndex = (frameSync.iCurrentFrameIndex + 1) % frameSync.vFences.size();

    calculateFrameStatistics();

#if defined(ENGINE_PROFILER_ENABLED)
    FrameMark;
#endif
}

void Renderer::drawGammaCorrectionScreenQuad(unsigned int iDrawFramebufferId, unsigned int iReadTextureId) {
    glBindFramebuffer(GL_FRAMEBUFFER, iDrawFramebufferId);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(pGammaCorrectionShaderProgram->getShaderProgramId());

    pGammaCorrectionShaderProgram->setFloatToShader("gamma", gamma);

    glDisable(GL_DEPTH_TEST);
    {
        // Bind rendered image.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, iReadTextureId);

        // Draw.
        glBindVertexArray(pFullscreenQuad->getVao().getVertexArrayObjectId());
        glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);

        // Reset texture slot.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glEnable(GL_DEPTH_TEST);
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
    pFullscreenQuad = nullptr;
    pGammaCorrectionShaderProgram = nullptr;
    pUiManager =
        nullptr; // UI manager uses some shaders programs while alive so destroy it before shader manager
    pFontManager = nullptr;

    for (auto& fence : frameSync.vFences) {
        glDeleteSync(fence);
    }

    // Delete framebuffers and stuff before OpenGL context.
    pPostProcessManager = nullptr;
    pMainFramebuffer = nullptr;
    
    pLightSourceManager = nullptr;
    pGammaCorrectionShaderProgram = nullptr;

    SDL_GL_DeleteContext(pContext);
}

void Renderer::waitForGpuToFinishWorkUpToThisPoint() { glFinish(); }

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

void Renderer::setGamma(float value) {
    gamma = std::clamp(value, 1.0F, 2.4F);

    // Save in the config.
    const auto pathToConfig = ProjectPaths::getPathToBaseConfigDirectory() / sGlobalRenderConfigFilename;
    ConfigManager config;
    config.setValue("", "gamma", gamma);

    const auto optionalError = config.saveFile(pathToConfig, false);
    if (optionalError.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to save render config file at \"{}\", error: {}",
            pathToConfig.string(),
            optionalError->getInitialMessage()));
    }
}

ShaderManager& Renderer::getShaderManager() { return *pShaderManager; }

LightSourceManager& Renderer::getLightSourceManager() { return *pLightSourceManager; }

UiManager& Renderer::getUiManager() { return *pUiManager; }

FontManager& Renderer::getFontManager() { return *pFontManager; }

TextureManager& Renderer::getTextureManager() { return *pTextureManager; }

RenderStatistics& Renderer::getRenderStatistics() { return renderStats; }

PostProcessManager& Renderer::getPostProcessManager() { return *pPostProcessManager; }
