#pragma once

#if defined(ENGINE_DEBUG_TOOLS)

// Custom.
#include "render/DebugDrawer.h"
#include "game/physics/CoordinateConversions.hpp"

// External.
#include "Jolt/Jolt.h"
#include "Jolt/Renderer/DebugRendererSimple.h"

/** Debug drawer for Jolt physics. */
class PhysicsDebugDrawer : public JPH::DebugRendererSimple {
public:
    PhysicsDebugDrawer() { Initialize(); }
    virtual ~PhysicsDebugDrawer() override {}

    /**
     * Changes debug rendering method.
     *
     * @param bWireframe `true` to draw as wireframe, `false` to draw as filled meshes.
     */
    void setDrawAsWireframe(bool bWireframe) { bDrawAsWireframe = bWireframe; }

    /**
     * Returns the current state of draw mode.
     *
     * @return Draw mode.
     */
    bool getDrawAsWireframe() const { return bDrawAsWireframe; }

    /**
     * Draws a line.
     *
     * @param from From.
     * @param to To.
     * @param color Color.
     */
    virtual void DrawLine(JPH::Vec3 from, JPH::Vec3 to, JPH::ColorArg color) override {
        vLinesToDraw.reserve(vLinesToDraw.size() + 2);

        vLinesToDraw.push_back(convertPosDirFromJolt(from));
        vLinesToDraw.push_back(convertPosDirFromJolt(to));
    }

    /**
     * Draws a triangle.
     *
     * @param v1 Vertex 1.
     * @param v2 Vertex 2.
     * @param v3 Vertex 3.
     * @param color Color.
     * @param castShadow Shadow state.
     */
    virtual void DrawTriangle(
        JPH::Vec3 v1, JPH::Vec3 v2, JPH::Vec3 v3, JPH::ColorArg color, ECastShadow castShadow) override {
        if (bDrawAsWireframe) {
            vLinesToDraw.push_back(convertPosDirFromJolt(v1));
            vLinesToDraw.push_back(convertPosDirFromJolt(v2));

            vLinesToDraw.push_back(convertPosDirFromJolt(v2));
            vLinesToDraw.push_back(convertPosDirFromJolt(v3));

            vLinesToDraw.push_back(convertPosDirFromJolt(v3));
            vLinesToDraw.push_back(convertPosDirFromJolt(v1));
        } else {
            vTrianglesToDraw.push_back(convertPosDirFromJolt(v1));
            vTrianglesToDraw.push_back(convertPosDirFromJolt(v2));
            vTrianglesToDraw.push_back(convertPosDirFromJolt(v3));
        }
    }

    /**
     * Draws spatial text.
     *
     * @param position Position of the text.
     * @param text     Text to display.
     * @param color    Color of the text.
     * @param height   Font height.
     */
    virtual void DrawText3D(
        JPH::Vec3 position,
        const JPH::string_view& text,
        JPH::ColorArg color = JPH::Color::sWhite,
        float height = 0.5f) override {
        Error::showErrorAndThrowException("not implemented");
    }

    /** Submits prepared render data for drawing. */
    void submitDrawData() {
        if (vLinesToDraw.empty() && vTrianglesToDraw.empty()) {
            return;
        }

        if (!vLinesToDraw.empty()) {
            DebugDrawer::drawLines(
                vLinesToDraw, glm::identity<glm::mat4x4>(), 0.0F, glm::vec4(collisionColor, 1.0F));
        }

        if (!vTrianglesToDraw.empty()) {
            DebugDrawer::drawMesh(
                vTrianglesToDraw,
                glm::identity<glm::mat4x4>(),
                0.0F,
                glm::vec4(collisionColor, 0.25F),
                false);
        }

        vLinesToDraw.clear(); // clear but don't shrink
        vTrianglesToDraw.clear();
    }

private:
    /** 2 positions per line to draw. */
    std::vector<glm::vec3> vLinesToDraw;

    /** 3 positions per triangle to draw. */
    std::vector<glm::vec3> vTrianglesToDraw;

    /** `true` to draw all as wireframe. */
    bool bDrawAsWireframe = false;

    /** Color of collision geometry. */
    const glm::vec3 collisionColor = glm::vec3(1.0F, 0.0F, 1.0F);
};

#endif
