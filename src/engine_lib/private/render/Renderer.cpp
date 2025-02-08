#include "render/Renderer.h"

// Custom.
#include "game/Window.h"

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

    // Set viewport.
    const auto windowSize = pWindow->getWindowSize();
    glViewport(0, 0, windowSize.first, windowSize.second);

    return std::unique_ptr<Renderer>(new Renderer(pWindow, pContext));
}

Renderer::Renderer(Window* pWindow, SDL_GLContext pCreatedContext) : pWindow(pWindow) {
    this->pContext = pCreatedContext;
}

void Renderer::drawNextFrame() {
    // Clear.
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(pWindow->getSdlWindow());
}

Renderer::~Renderer() { SDL_GL_DeleteContext(pContext); }

void Renderer::waitForGpuToFinishWorkUpToThisPoint() { glFinish(); }
