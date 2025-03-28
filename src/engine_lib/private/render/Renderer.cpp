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

    return std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;

    pLightSourceManager = std::unique_ptr<LightSourceManager>(new LightSourceManager(this));
    pUiManager = std::unique_ptr<UiManager>(new UiManager(this));
    pFontManager = FontManager::create(
        this,
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "RedHatDisplay-Light.ttf");
}

void Renderer::drawNextFrame() {
    // Make sure there was no GL error during the last frame.
    const auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("an OpenGL error occurred during the last frame, error code: {}", lastError));
    }

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

    // Swap.
    SDL_GL_SwapWindow(pWindow->getSdlWindow());
}

Renderer::~Renderer() { SDL_GL_DeleteContext(pContext); }

void Renderer::waitForGpuToFinishWorkUpToThisPoint() { glFinish(); }

Window* Renderer::getWindow() const { return pWindow; }

GpuResourceManager& Renderer::getGpuResourceManager() { return gpuResourceManager; }

ShaderManager& Renderer::getShaderManager() { return shaderManager; }

LightSourceManager& Renderer::getLightSourceManager() { return *pLightSourceManager.get(); }

UiManager& Renderer::getUiManager() { return *pUiManager.get(); }

FontManager& Renderer::getFontManager() { return *pFontManager.get(); }
