#pragma once

#if defined(ENGINE_DEBUG_TOOLS)

// Standard.
#include <vector>
#include <memory>
#include <optional>

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
        /** Color of the mesh. */
        glm::vec3 color = glm::vec3(1.0F, 0.0F, 0.0F);

        /** World matrix to transform @ref vTrianglePositions. */
        glm::mat4x4 worldMatrix = glm::identity<glm::mat4x4>();

        /** Time after which the mesh should no longer be rendered. */
        float timeLeftSec = 0.0F;

        /** VAO used for drawing the mesh. */
        std::unique_ptr<VertexArrayObject> pVao;
    };

    /** Data used to draw text. */
    struct Text {
        /** Text to draw. */
        std::string sText = "text";

        /** Height of the text in range [0.0; 1.0] relative to screen height. */
        float textHeight = 0.1F;

        /**
         * If empty the text will appear in the corner of the screen and new
         * text will be automatically displayed below already existing text (so multiple
         * text object won't be drawn on top of each other), otherwise if specified describes
         * the position of the top-left corner of the text in range [0.0; 1.0] relative to screen.
         */
        std::optional<glm::vec2> optForcePosition = {};

        /** Time after which the mesh should no longer be rendered. */
        float timeLeftSec = 3.0F;

        /** Color of the text. */
        glm::vec3 color = glm::vec3(1.0F, 1.0F, 1.0F);
    };

    /** Data used to draw a rectangle on the screen. */
    struct ScreenRect {
        /** Position of the top-left corner in range [0.0; 1.0] relative to screen. */
        glm::vec2 screenPos = glm::vec2(0.1F, 0.1F);

        /** Width and height in range [0.0; 1.0] relative to screen. */
        glm::vec2 screenSize = glm::vec2(0.25F, 0.25F);

        /** Time after which the mesh should no longer be rendered. */
        float timeLeftSec = 0.0F;

        /** Color of the rectangle. */
        glm::vec3 color = glm::vec3(1.0F, 1.0F, 1.0F);
    };

    DebugDrawer(const DebugDrawer&) = delete;
    DebugDrawer& operator=(const DebugDrawer&) = delete;
    ~DebugDrawer();

    /**
     * Draws a cube.
     *
     * @param size      Full size (both sides) of the cube on one axis.
     * @param worldPosition Position of the center of the cube in the world.
     * @param timeInSec Time (in seconds) during which the mesh will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color     Color of the mesh.
     */
    static void drawCube(
        float size,
        const glm::vec3& worldPosition,
        float timeInSec,
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
    static void drawSphere(
        float radius,
        const glm::vec3& worldPosition,
        float timeInSec,
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
    static void drawMesh(
        const std::vector<glm::vec3>& vTrianglePositions,
        const glm::mat4x4& worldMatrix,
        float timeInSec,
        const glm::vec3& color = glm::vec3(1.0F, 0.0F, 0.0F));

    /**
     * Draws a lines.
     *
     * @param vLines      2 positions per line.
     * @param worldMatrix World matrix to transform lines.
     * @param timeInSec   Time (in seconds) during which the lines will be rendered
     * then they will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color       Color of the lines.
     */
    static void drawLines(
        const std::vector<glm::vec3>& vLines,
        const glm::mat4x4& worldMatrix,
        float timeInSec,
        const glm::vec3& color = glm::vec3(1.0F, 0.0F, 0.0F));

    /**
     * Draws text on the screen.
     *
     * @param sText      Text to draw.
     * @param timeInSec  Time (in seconds) during which the text will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     * @param color      Color of the text.
     * @param optPos     If empty the text will appear in the corner of the screen and new
     * text will be automatically displayed below already existing text (so multiple
     * text object won't be drawn on top of each other), otherwise you can specify
     * a position of the top-left corner of the text in range [0.0; 1.0] relative to screen.
     * @param textHeight Height of the text in range [0.0; 1.0] relative to screen height.
     */
    static void drawText(
        const std::string& sText,
        float timeInSec,
        const glm::vec3& color = glm::vec3(1.0F, 1.0F, 1.0F),
        const std::optional<glm::vec2>& optForcePosition = {},
        float textHeight = 0.0325F);

    /**
     * Draws a 2D rectangle on the screen.
     *
     * @param screenPos  Position of the top-left corner in range [0.0; 1.0] relative to screen.
     * @param screenSize Width and height in range [0.0; 1.0] relative to screen.
     * @param color      Color of the rectangle.
     * @param timeInSec  Time (in seconds) during which the text will be rendered
     * then it will be removed from rendering. Specify 0.0 to draw for a single frame.
     */
    static void drawScreenRect(
        const glm::vec2& screenPos, const glm::vec2& screenSize, const glm::vec3& color, float timeInSec);

private:
    /** Groups info about shader program for rendering rectangles. */
    struct RectShaderProgram {
        /** Shader program used for rendering rectangle. */
        std::shared_ptr<ShaderProgram> pShaderProgram;

        /** Location of a shader uniform variable. */
        int iScreenPosUniform = 0;

        /** Location of a shader uniform variable. */
        int iScreenSizeUniform = 0;

        /** Location of a shader uniform variable. */
        int iClipRectUniform = 0;

        /** Location of a shader uniform variable. */
        int iWindowSizeUniform = 0;
    };

    /** Groups info about shader program for rendering text. */
    struct TextShaderProgram {
        /** Shader program used for rendering text. */
        std::shared_ptr<ShaderProgram> pShaderProgram;

        /** Location of a shader uniform variable. */
        int iScreenPosUniform = 0;

        /** Location of a shader uniform variable. */
        int iScreenSizeUniform = 0;

        /** Location of a shader uniform variable. */
        int iClipRectUniform = 0;

        /** Location of a shader uniform variable. */
        int iWindowSizeUniform = 0;
    };

    DebugDrawer();

    /**
     * Returns a reference to the debug drawer instance.
     * If no instance was created yet, this function will create it
     * and return a reference to it.
     *
     * @return Reference to the instance.
     */
    static DebugDrawer& get();

    /**
     * Called by the renderer to draw all available debug objects.
     *
     * @param pRenderer               Renderer.
     * @param viewProjectionMatrix    Camera's view projection matrix.
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    void drawDebugObjects(
        Renderer* pRenderer, const glm::mat4& viewProjectionMatrix, float timeSincePrevFrameInSec);

    /** Destroys used render resources such as @ref pMeshShaderProgram and removes any geometry to render. */
    void destroy();

    /**
     * Draws an quad in screen (window) coordinates.
     *
     * @param iScreenPosUniform Location of the uniform variable.
     * @param iScreenSizeUniform Location of the uniform variable.
     * @param iClipRectUniform Location of the uniform variable.
     * @param iWindowSizeUniform Location of the uniform variable.
     * @param screenPos     Position of the top-left corner of the quad.
     * @param screenSize    Size of the quad.
     * @param iWindowWidth Width of the window.
     * @param iWindowHeight Height of the window.
     */
    void drawQuad(
        int iScreenPosUniform,
        int iScreenSizeUniform,
        int iClipRectUniform,
        int iWindowSizeUniform,
        glm::vec2 screenPos,
        glm::vec2 screenSize,
        unsigned int iWindowWidth,
        unsigned int iWindowHeight) const;

    /** Precalculated positions to draw an icosphere. */
    std::vector<glm::vec3> vIcospherePositions;

    /** Precalculated positions to draw a cube. */
    std::vector<glm::vec3> vCubePositions;

    /** Meshes to draw on the next frame. */
    std::vector<Mesh> vMeshesToDraw;

    /** Text to draw on the next frame. */
    std::vector<Text> vTextToDraw;

    /** Screen rectangles to draw on the next frame. */
    std::vector<ScreenRect> vRectsToDraw;

    /** Shader program used to draw meshes. */
    std::shared_ptr<ShaderProgram> pMeshShaderProgram;

    /** Shader program used to draw screen rectangles. */
    RectShaderProgram rectShaderInfo;

    /** Shader program used to draw text. */
    TextShaderProgram textShaderInfo;

    /** Quad used for rendering text. */
    std::unique_ptr<ScreenQuadGeometry> pScreenQuadGeometry;

    /** Uniform location for @ref pMeshShaderProgram. */
    int iMeshProgramViewProjectionMatrixUniform = 0;

    /** `true` if @ref destroy was called. */
    bool bIsDestroyed = false;
};

#endif
