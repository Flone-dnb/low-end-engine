#pragma once

// Custom.
#include "game/node/SpatialNode.h"
#include "math/GLMath.hpp"
#include "render/ShaderAlignmentConstants.hpp"
#include "render/shader/LightSourceShaderArray.h"
#include "game/geometry/shapes/Frustum.h"
#include "game/geometry/shapes/Cone.h"

class ShaderArrayIndex;
class Framebuffer;

/** Cone-shaped light source */
class SpotlightNode : public SpatialNode {
public:
    /** Data that will be directly copied to shaders. */
    struct ShaderProperties {
        ShaderProperties();

        /** Matrix used for shadow mapping. */
        alignas(ShaderAlignmentConstants::iMat4) glm::mat4 viewProjectionMatrix;

        /** Light position in world space. 4th component is not used. */
        alignas(ShaderAlignmentConstants::iVec4) glm::vec4 position = glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);

        /** Forward unit vector in the direction of the light source. 4th component is not used. */
        alignas(ShaderAlignmentConstants::iVec4) glm::vec4 direction = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);

        /** Light color and 4th component stores intensity in range [0.0; 1.0]. */
        alignas(ShaderAlignmentConstants::iVec4) glm::vec4 colorAndIntensity =
            glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);

        /** Lit distance. */
        alignas(ShaderAlignmentConstants::iScalar) float distance = 10.0F;

        /**
         * Cosine of the spotlight's inner cone angle (cutoff).
         *
         * @remark Represents cosine of the cutoff angle on one side from the light direction
         * (not both sides), i.e. this is a cosine of value [0-90] degrees.
         */
        alignas(ShaderAlignmentConstants::iScalar) float cosInnerConeAngle = 0.0F;

        /**
         * Cosine of the spotlight's outer cone angle (cutoff).
         *
         * @remark Represents cosine of the cutoff angle on one side from the light direction
         * (not both sides), i.e. this is a cosine of value [0-90] degrees.
         */
        alignas(ShaderAlignmentConstants::iScalar) float cosOuterConeAngle = 0.0F;

        /** -1 if shadow casting is disabled. */
        alignas(ShaderAlignmentConstants::iScalar) int iShadowMapIndex = -1;
    };

    /** Groups data for shadow pass. */
    struct ShadowMapData {
        /** Framebuffer with shadow map. */
        std::unique_ptr<Framebuffer> pFramebuffer;

        /** Index into the shader array of shadow maps. */
        std::unique_ptr<ShaderArrayIndex> pIndex;

        /** View matrix for shadow pass. */
        glm::mat4 viewMatrix;

        /** Light's frustum in world space. */
        Frustum frustumWorld;
    };

    SpotlightNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    SpotlightNode(const std::string& sNodeName);

    virtual ~SpotlightNode() override = default;

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
     * Returns the maximum angle for @ref getLightInnerConeAngle and @ref getLightOuterConeAngle.
     *
     * @return Maximum cone angle (in degrees).
     */
    static constexpr float getMaxLightConeAngle() { return maxConeAngle; }

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
     * Sets lit distance (i.e. attenuation distance).
     *
     * @param distance Lit distance.
     */
    void setLightDistance(float distance);

    /**
     * Sets angle of spotlight's inner cone (cone that will have hard light edges),
     * see @ref setLightOuterConeAngle for configuring soft light edges.
     *
     * @param inInnerConeAngle Angle in degrees in range [0.0; @ref getMaxLightConeAngle] (will be clamped
     * if outside of the range).
     */
    void setLightInnerConeAngle(float inInnerConeAngle);

    /**
     * Sets angle of spotlight's inner cone (cone that will have hard light edges),
     * see @ref setLightOuterConeAngle for configuring soft light edges.
     *
     * @param inOuterConeAngle Angle in degrees in range [@ref getLightInnerConeAngle; @ref
     * getMaxLightConeAngle] (will be clamped if outside of the range).
     */
    void setLightOuterConeAngle(float inOuterConeAngle);

    /**
     * Enables/disables casted shadows.
     *
     * @param bEnable `true` to enable shadows.
     */
    void setCastShadows(bool bEnable);

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
     * Returns light cutoff angle of the inner cone (hard light edge).
     *
     * @return Angle in degrees in range [0.0; @ref getMaxLightConeAngle].
     */
    float getLightInnerConeAngle() const { return innerConeAngle; }

    /**
     * Returns light cutoff angle of the outer cone (soft light edge).
     *
     * @return Angle in degrees in range [@ref getLightInnerConeAngle; @ref getMaxLightConeAngle].
     */
    float getLightOuterConeAngle() const { return outerConeAngle; }

    /**
     * `true` if this light source is included in the rendering, `false` otherwise.
     *
     * @return Visibility.
     */
    bool isVisible() const { return bIsVisible; }

    /**
     * Tells if this light source casts shadows.
     *
     * @return `true` if enabled.
     */
    bool isCastingShadows() const { return bCastShadows; }

    /**
     * Returns shape of the light source in world space.
     *
     * @return Shape.
     */
    const Cone& getConeShapeWorld() const { return coneWorld; }

    /**
     * Returns `nullptr` if shadow data not created yet.
     *
     * @return Internal shadow data used for shadow pass.
     */
    ShadowMapData* getInternalShadowMapData() const { return pShadowMapData.get(); }

    /**
     * Returns view projection matrix that transforms to light's space.
     *
     * @return Matrix.
     */
    const glm::mat4& getLightViewProjectionMatrix() const { return shaderProperties.viewProjectionMatrix; }

    /**
     * Returns internal light source handle.
     *
     * @return `nullptr` if the light is not registered for rendering.
     */
    ActiveLightSourceHandle* getInternalLightSourceHandle() const { return pActiveLightHandle.get(); }

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
    /** Initializes @ref pShadowMapData. */
    void createShadowMapData();

    /** Recalculates projection matrix for shadow mapping. */
    void recalculateShadowProjMatrix();

    /** Recalculates @ref coneWorld. */
    void recalculateConeShape();

    /** Data to copy to shaders. */
    ShaderProperties shaderProperties;

    /** Not `nullptr` if @ref bCastShadows is enabled. */
    std::unique_ptr<ShadowMapData> pShadowMapData;

    /** Not `nullptr` if being rendered. */
    std::unique_ptr<ActiveLightSourceHandle> pActiveLightHandle;

    /** Light's cone shape in world space. */
    Cone coneWorld;

    /**
     * Light cutoff angle (in degrees) of the inner cone (hard light edge).
     * Valid values range is [0.0F, @ref maxConeAngle].
     */
    float innerConeAngle = 25.0F;

    /**
     * Light cutoff angle (in degrees) of the outer cone (soft light edge).
     * Valid values range is [@ref innerConeAngle, @ref maxConeAngle].
     */
    float outerConeAngle = 45.0F;

    /** Enabled for rendering or not. */
    bool bIsVisible = true;

    /** `true` to enable shadows. */
    bool bCastShadows = false;

    /** Maximum value for @ref innerConeAngle and @ref outerConeAngle. */
    static constexpr float maxConeAngle = 80.0F; // max angle that won't cause any visual issues
};
