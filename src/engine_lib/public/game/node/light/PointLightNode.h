#pragma once

// Custom.
#include "game/node/SpatialNode.h"
#include "math/GLMath.hpp"
#include "render/ShaderAlignmentConstants.hpp"
#include "render/shader/LightSourceShaderArray.h"
#include "game/geometry/shapes/Sphere.h"

/** Sphere-shaped light source. */
class PointLightNode : public SpatialNode {
public:
    /** Data that will be directly copied to shaders. */
    struct ShaderProperties {
        ShaderProperties();

        /** Light position in world space. 4th component is not used. */
        alignas(ShaderAlignmentConstants::iVec4) glm::vec4 position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        /** Light color and 4th component stores intensity in range [0.0; 1.0] */
        alignas(ShaderAlignmentConstants::iVec4) glm::vec4 colorAndIntensity =
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        /** Lit distance. Radius of the sphere. */
        alignas(ShaderAlignmentConstants::iScalar) float distance = 15.0f;

        /** Padding to 16 bytes. */
        float pad[3];
    };

    PointLightNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    PointLightNode(const std::string& sNodeName);

    virtual ~PointLightNode() override = default;

    /**
     * Returns reflection info about this type.
     *
     * @return Type reflection.
     */
    static TypeReflectionInfo getReflectionInfo();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    static std::string getTypeGuidStatic();

    /**
     * Returns GUID of the type, this GUID is used to retrieve reflection information from the reflected type
     * database.
     *
     * @return GUID.
     */
    virtual std::string getTypeGuid() const override;

    /**
     * Sets whether this light source will be included in the rendering or not.
     *
     * @param bNewVisible New visibility.
     */
    void setIsVisible(bool bNewVisible);

    /**
     * Sets light's color.
     *
     * @param color Color in RGB format in range [0.0; 1.0].
     */
    void setLightColor(const glm::vec3& color);

    /**
     * Sets light's intensity.
     *
     * @param intensity Intensity in range [0.0; 1.0] (will be clamped if outside of the range).
     */
    void setLightIntensity(float intensity);

    /**
     * Sets lit distance (i.e. attenuation radius).
     *
     * @param distance Lit distance.
     */
    void setLightDistance(float distance);

    /**
     * Returns color of this light source.
     *
     * @return Color in RGB format in range [0.0; 1.0].
     */
    glm::vec3 getLightColor() const { return shaderProperties.colorAndIntensity; }

    /**
     * Returns intensity of this light source.
     *
     * @return Intensity in range [0.0; 1.0].
     */
    float getLightIntensity() const { return shaderProperties.colorAndIntensity.w; }

    /**
     * Returns lit distance.
     *
     * @return Distance.
     */
    float getLightDistance() const { return shaderProperties.distance; }

    /**
     * `true` if this light source is included in the rendering, `false` otherwise.
     *
     * @return Visibility.
     */
    bool isVisible() const { return bIsVisible; }

    /**
     * Returns internal light source handle.
     *
     * @return `nullptr` if the light is not registered for rendering.
     */
    ActiveLightSourceHandle* getInternalLightSourceHandle() const { return pActiveLightHandle.get(); }

    /**
     * Returns shape of the light source in world space.
     *
     * @return Shape.
     */
    const Sphere& getSphereShapeWorld() const { return sphereShapeWorld; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node to execute custom spawn logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     *
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onSpawning() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onDespawning() override;

    /**
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark If you change location/rotation/scale inside of this function,
     * this function will not be called again (no recursion will occur).
     */
    virtual void onWorldLocationRotationScaleChanged() override;

private:
    /** Data to copy to shaders. */
    ShaderProperties shaderProperties;

    /** Not `nullptr` if being rendered. */
    std::unique_ptr<ActiveLightSourceHandle> pActiveLightHandle;

    /** Shape of the light source in world space. */
    Sphere sphereShapeWorld;

    /** Enabled for rendering or not. */
    bool bIsVisible = true;
};
