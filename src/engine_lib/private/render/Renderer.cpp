#include "render/Renderer.h"

// Custom.
#include "game/Window.h"
#include "render/ShaderProgram.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "game/node/MeshNode.h"

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
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glClearDepthf(1.0F);

    // Enable depth testing.
    GL_CHECK_ERROR(glEnable(GL_DEPTH_TEST));

    // Setup clear color.
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    // Disable VSync.
    if (SDL_GL_SetSwapInterval(0) < 0) [[unlikely]] {
        Error error(SDL_GetError());
        error.showErrorAndThrowException();
    }

    // Set viewport.
    const auto windowSize = pWindow->getWindowSize();
    GL_CHECK_ERROR(glViewport(0, 0, windowSize.first, windowSize.second));

    return std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;
}

void Renderer::drawNextFrame() {
    // Make sure there was no GL error during the last frame.
    const auto lastError = glGetError();
    if (lastError != GL_NO_ERROR) [[unlikely]] {
        Error error(std::format("an OpenGL error occurred during the last frame, error code: {}", lastError));
        error.showErrorAndThrowException();
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

        // Draw meshes for each shader program.
        for (auto& [sProgramName, shaderProgram] : mtxShaderPrograms.second) {
            const auto& [pWeakPtr, pShaderProgram] = shaderProgram;

            // Set shader program.
            glUseProgram(pShaderProgram->getShaderProgramId());

            // Set camera uniforms.
            mtxActiveCamera.second->getCameraProperties()->getShaderConstantsManager().setConstantsToShader(
                pShaderProgram);

            // Get mesh nodes.
            auto& mtxMeshNodes = pShaderProgram->getMeshNodesUsingThisProgram();
            std::scoped_lock guard(mtxMeshNodes.first);

            for (const auto& pMeshNode : mtxMeshNodes.second) {
                // Set mesh.
                auto& vao = pMeshNode->getVertexArrayObjectWhileSpawned();
                glBindVertexArray(vao.getVertexArrayObjectId());

                // Set mesh uniforms.
                pMeshNode->getShaderConstantsManagerWhileSpawned().setConstantsToShader(pShaderProgram);

                // Draw.
                glDrawElements(GL_TRIANGLES, vao.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);
            }
        }
    }

    // Swap.
    SDL_GL_SwapWindow(pWindow->getSdlWindow());
}

Renderer::~Renderer() { SDL_GL_DeleteContext(pContext); }

void Renderer::waitForGpuToFinishWorkUpToThisPoint() { glFinish(); }

ShaderManager& Renderer::getShaderManager() { return shaderManager; }
