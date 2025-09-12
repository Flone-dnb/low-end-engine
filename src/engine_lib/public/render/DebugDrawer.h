#pragma once

#if defined(DEBUG)

// Standard.
#include <vector>
#include <memory>

// Custom.
#include "render/wrapper/VertexArrayObject.h"
#include "game/geometry/ScreenQuadGeometry.h"
#include "math/GLMath.hpp"

class ShaderProgram;
class CameraProperties;
class Renderer;

/** Used to draw temporary objects for debugging purposes. */
class DebugDrawer {
    // Renderer notifies when it's time to draw debug objects.
    friend class Renderer;

public:
    /** Data used to draw a mesh. */
    struct Mesh {
        /** 3 positions per triangle. */
        std::vector<glm::vec3> vTrianglePositions;

        /** Color of the mesh. */
        glm::vec3 color = glm::vec3(1.0F, 0.0F, 0.0F);

        /** World matrix to transform @ref vTrianglePositions. */
        glm::mat4x4 worldMatrix = glm::identity<glm::mat4x4>();

        /** Time after which the mesh should no longer be rendered. */
        float timeLeftSec = 3.0F;
    };

    /** Data used to draw text. */
    struct Text {
        /** Text to draw. */
        std::string sText = "text";

        /** Height of the text in range [0.0; 1.0] relative to screen height. */
        float textHeight = 0.1F;

        /** Time after which the mesh should no longer be rendered. */
        float timeLeftSec = 3.0F;

        /** Color of the text. */
        glm::vec3 color = glm::vec3(1.0F, 1.0F, 1.0F);
    };

    DebugDrawer(const DebugDrawer&) = delete;
    DebugDrawer& operator=(const DebugDrawer&) = delete;
    ~DebugDrawer();

    /**
     * Returns a reference to the debug drawer instance.
     * If no instance was created yet, this function will create it
     * and return a reference to it.
     *
     * @return Reference to the logger instance.
     */
    static DebugDrawer& get();

    /**
     * Draws a cube.
     *
     * @param size      Full size (both sides) of the cube on one axis.
     * @param worldPosition Position of the center of the cube in the world.
     * @param timeInSec Time (in seconds) during which the mesh will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color     Color of the mesh.
     */
    void drawCube(
        float size,
        const glm::vec3& worldPosition,
        float timeInSec = 0.0F,
        const glm::vec3& color = glm::vec3(1.0F, 0.0F, 0.0F));

    /**
     * Draws a sphere.
     *
     * @param radius    Radius of the sphere.
     * @param worldPosition Position of the center of the sphere in the world.
     * @param timeInSec Time (in seconds) during which the mesh will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color     Color of the mesh.
     */
    void drawSphere(
        float radius,
        const glm::vec3& worldPosition,
        float timeInSec = 0.0F,
        const glm::vec3& color = glm::vec3(1.0F, 0.0F, 0.0F));

    /**
     * Draws a triangle mesh.
     *
     * @param vTrianglePositions 3 positions per triangle.
     * @param worldMatrix        World matrix to transform triangles.
     * @param timeInSec          Time (in seconds) during which the mesh will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color              Color of the mesh.
     */
    void drawMesh(
        const std::vector<glm::vec3>& vTrianglePositions,
        const glm::mat4x4& worldMatrix = glm::identity<glm::mat4x4>(),
        float timeInSec = 0.0F,
        const glm::vec3& color = glm::vec3(1.0F, 0.0F, 0.0F));

    /**
     * Draws text on the screen.
     *
     * @param sText      Text to draw.
     * @param timeInSec  Time (in seconds) during which the text will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color      Color of the text.
     * @param textHeight Height of the text in range [0.0; 1.0] relative to screen height.
     */
    void drawText(
        const std::string& sText,
        float timeInSec = 3.0F,
        const glm::vec3& color = glm::vec3(1.0F, 1.0F, 1.0F),
        float textHeight = 0.035F);

private:
    DebugDrawer();

    /**
     * Called by the renderer to draw all available debug objects.
     *
     * @param pRenderer               Renderer.
     * @param pCameraProperties       Properties of the active camera.
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    void
    drawDebugObjects(Renderer* pRenderer, CameraProperties* pCameraProperties, float timeSincePrevFrameInSec);

    /** Destroys used render resources such as @ref pMeshShaderProgram and removes any geometry to render. */
    void destroy();

    /** Precalculated positions to draw an icosphere. */
    std::vector<glm::vec3> vIcospherePositions;

    /** Precalculated positions to draw a cube. */
    std::vector<glm::vec3> vCubePositions;

    /** Meshes to draw on the next frame. */
    std::vector<Mesh> vMeshesToDraw;

    /** Text to draw on the next frame. */
    std::vector<Text> vTextToDraw;

    /** Shader program used to draw geometry. */
    std::shared_ptr<ShaderProgram> pMeshShaderProgram;

    /** Shader program used to draw text. */
    std::shared_ptr<ShaderProgram> pTextShaderProgram;

    /** VAO used for drawing. */
    std::unique_ptr<VertexArrayObject> pVao;

    /** Quad used for rendering text. */
    std::unique_ptr<ScreenQuadGeometry> pScreenQuadGeometry;

    /** Orthographic projection matrix for rendering UI elements. */
    glm::mat4 uiProjMatrix;

    /** `true` if @ref destroy was called. */
    bool bIsDestroyed = false;
};

#endif
