#pragma once

#if defined(DEBUG)

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
     * Draws a line.
     *
     * @param from From.
     * @param to To.
     * @param color Color.
     */
    virtual void DrawLine(JPH::Vec3 from, JPH::Vec3 to, JPH::ColorArg color) override {
        Error::showErrorAndThrowException("not implemented");
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
        vTrianglePositions.push_back(convertFromJolt(v1));
        vTrianglePositions.push_back(convertFromJolt(v2));
        vTrianglePositions.push_back(convertFromJolt(v3));
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
        if (vTrianglePositions.empty()) {
            return;
        }
        // TODO: consider implementing DebugRenderer (not DebugRendererSimple)
        DebugDrawer::get().drawMesh(
            vTrianglePositions,
            glm::identity<glm::mat4x4>(),
            0.0F,
            glm::vec3(1.0F));
        vTrianglePositions.clear();
    }

private:
    /** Triangle positions to draw. */
    std::vector<glm::vec3> vTrianglePositions;
};

#endif
