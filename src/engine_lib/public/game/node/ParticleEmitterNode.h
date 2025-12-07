#pragma once

// Standard.
#include <vector>
#include <random>
#include <string>
#include <memory>

// Custom.
#include "game/node/SpatialNode.h"
#include "render/wrapper/Texture.h"
#include "math/GLMath.hpp"

class ParticleRenderingHandle;
class TextureHandle;

/** Emits particles. */
class ParticleEmitterNode : public SpatialNode {
public:
    ParticleEmitterNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    ParticleEmitterNode(const std::string& sNodeName);

    virtual ~ParticleEmitterNode() override;

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
     * Sets time in seconds between new batch of particles @ref iParticleCountPerSpawn is spawned.
     *
     * @param delay Time.
     */
    void setDelayBetweenSpawns(float delay);

    /**
     * Sets the maximum time in seconds randomly added to @ref getDelayBetweenSpawns.
     *
     * @param delay Time.
     */
    void setDelayBetweenSpawnsMaxAdd(float delay) { delayBetweenSpawnsMaxAdd = std::max(0.0f, delay); }

    /**
     * Sets time in seconds before a particle is destroyed.
     *
     * @param time Time.
     */
    void setTimeToLive(float time);

    /**
     * Sets the maximum time in seconds randomly added to @ref getTimeToLive.
     *
     * @param time Time.
     */
    void setTimeToLiveMaxAdd(float time);

    /**
     * Sets the number of particles to spawn after each @ref getDelayBetweenSpawns.
     *
     * @param iParticleCount Particle count.
     */
    void setParticleCountPerSpawn(unsigned int iParticleCount);

    /**
     * Sets the maximum number of particles that will be randomly added to @ref getParticleCountPerSpawn.
     *
     * @param iParticleCount Particle count.
     */
    void setParticleMaxAddCountPerSpawn(unsigned int iParticleCount);

    /**
     * Sets velocity of spawned particles.
     *
     * @param newVelocity Velocity.
     */
    void setSpawnVelocity(const glm::vec3& newVelocity) { spawnVelocity = newVelocity; }

    /**
     * Sets a value where each component stores a non-negative value that will be randomely added
     * to @ref setSpawnVelocity as [-value; +value].
     *
     * @param newVelocity Velocity randomization.
     */
    void setSpawnVelocityRandomization(const glm::vec3& newVelocity) {
        spawnVelocityRandomization = glm::max(glm::vec3(0.0f), newVelocity);
    }

    /**
     * Sets optional gravity to affect particles.
     *
     * @param newGravity Gravity.
     */
    void setGravity(const glm::vec3& newGravity) { gravity = newGravity; }

    /**
     * Sets particle color.
     *
     * @param newColor New color.
     */
    void setColor(const glm::vec4& newColor) { color = newColor; }

    /**
     * Each component stores a non-negative value that will be randomely added
     * to @ref setColor as [-value; +value].
     *
     * @param newColor New color.
     */
    void setColorRandomization(const glm::vec3& newColor) {
        colorRandomization = glm::max(glm::vec3(0.0f), newColor);
    }

    /**
     * Sets color that particle should have when fading in (see @ref setFadeInLifePortion).
     *
     * @param newColor Color.
     */
    void setColorFadeIn(const glm::vec4& newColor) { colorFadeIn = newColor; }

    /**
     * Sets color that particle should have when fading out (see @ref setFadeOutLifePortion).
     *
     * @param newColor Color.
     */
    void setColorFadeOut(const glm::vec4& newColor) { colorFadeOut = newColor; }

    /**
     * Sets path (relative to the `res` directory) to the texture used by particles.
     *
     * @param sNewRelativePathToTexture Empty string to not use a texture, otherwise a valid path.
     */
    void setRelativePathToTexture(std::string sNewRelativePathToTexture);

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     */
    void setFadeInLifePortion(float portion) { fadeInLifePortion = std::clamp(portion, 0.0f, 1.0f); }

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     */
    void setFadeOutLifePortion(float portion) { fadeOutLifePortion = std::clamp(portion, 0.0f, 1.0f); }

    /**
     * Sets if the simulation should be paused.
     *
     * @param bPaused `true` to pause.
     */
    void setIsPaused(bool bPaused) { bIsPaused = bPaused; }

    /**
     * Sets particle size.
     *
     * @param newSize Size.
     */
    void setSize(float newSize) { size = std::max(0.01f, newSize); }

    /**
     * Sets particle size when fading in.
     *
     * @param newSize Size.
     */
    void setSizeFadeIn(float newSize) { sizeFadeIn = std::max(0.01f, newSize); }

    /**
     * Sets particle size when fading out.
     *
     * @param newSize Size.
     */
    void setSizeFadeOut(float newSize) { sizeFadeOut = std::max(0.01f, newSize); }

    /**
     * Returns velocity of spawned particles.
     *
     * @return Velocity.
     */
    glm::vec3 getSpawnVelocity() const { return spawnVelocity; }

    /**
     * Returns value where each component stores a non-negative value that will be randomely added
     * to @ref getSpawnVelocity as [-value; +value].
     *
     * @return Velocity randomization.
     */
    glm::vec3 getSpawnVelocityRandomization() const { return spawnVelocityRandomization; }

    /**
     * Returns optional gravity to affect particles.
     *
     * @return Gravity.
     */
    glm::vec3 getGravity() const { return gravity; }

    /**
     * Returns color of particles.
     *
     * @return Color.
     */
    glm::vec4 getColor() const { return color; }

    /**
     * Each component stores a non-negative value that will be randomely added
     * to @ref getColor as [-value; +value].
     *
     * @return Color.
     */
    glm::vec3 getColorRandomization() const { return colorRandomization; }

    /**
     * Color that particle should have when fading in (see @ref getFadeInLifePortion).
     *
     * @return Color.
     */
    glm::vec4 getColorFadeIn() const { return colorFadeIn; }

    /**
     * Color that particle should have when fading out (see @ref getFadeOutLifePortion).
     *
     * @return Color.
     */
    glm::vec4 getColorFadeOut() const { return colorFadeOut; }

    /**
     * Returns path (relative to the `res` directory) used by particles.
     *
     * @return Empty if not used.
     */
    std::string getRelativePathToTexture() const { return sRelativePathToTexture; }

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     *
     * @return Lifetime portion.
     */
    float getFadeInLifePortion() const { return fadeInLifePortion; }

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     *
     * @return Lifetime portion.
     */
    float getFadeOutLifePortion() const { return fadeOutLifePortion; }

    /**
     * Returns particle size.
     *
     * @return Size.
     */
    float getSize() const { return size; }

    /**
     * Returns particle size when fading in.
     *
     * @return Size.
     */
    float getSizeFadeIn() const { return sizeFadeIn; }

    /**
     * Returns particle size when fading out.
     *
     * @return Size.
     */
    float getSizeFadeOut() const { return sizeFadeOut; }

    /**
     * Returns the number of particles to spawn after each @ref getDelayBetweenSpawns.
     *
     * @return Particle count.
     */
    unsigned int getParticleCountPerSpawn() const { return iParticleCountPerSpawn; }

    /**
     * Returns the maximum number of particles that will be randomly added to @ref getParticleCountPerSpawn.
     *
     * @return Particle count.
     */
    unsigned int getParticleMaxAddCountPerSpawn() const { return iParticleMaxAddCountPerSpawn; }

    /**
     * Returns time in seconds between new batch of particles @ref iParticleCountPerSpawn is spawned.
     *
     * @return Time.
     */
    float getDelayBetweenSpawns() const { return delayBetweenSpawns; }

    /**
     * Returns maximum time in seconds randomly added to @ref getDelayBetweenSpawns.
     *
     * @return Time.
     */
    float getDelayBetweenSpawnsMaxAdd() const { return delayBetweenSpawnsMaxAdd; }

    /**
     * Returns time in seconds before a particle is destroyed.
     *
     * @return Time.
     */
    float getTimeToLive() const { return timeToLive; }

    /**
     * Returns the maximum time in seconds randomly added to @ref getTimeToLive.
     *
     * @return Time.
     */
    float getTimeToLiveMaxAdd() const { return timeToLiveMaxAdd; }

    /**
     * Returns `true` if simulation is paused.
     *
     * @return State.
     */
    bool isPaused() const { return bIsPaused; }

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
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() override;

    /**
     * Called before a new frame is rendered.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic (if there is any).
     *
     * @remark This function is disabled by default, use @ref setIsCalledEveryFrame to enable it.
     * @remark This function will only be called while this node is spawned.
     *
     * @param timeSincePrevFrameInSec Also known as deltatime - time in seconds that has passed since
     * the last frame was rendered.
     */
    virtual void onBeforeNewFrame(float timeSincePrevFrameInSec) override;

private:
    /** Information about a single particle. */
    struct ParticleData {
        /** Current world position. */
        glm::vec3 position = glm::vec3(0.0f);

        /** Current velocity. */
        glm::vec3 velocity = glm::vec3(0.0f);

        /** Current color (including alpha). */
        glm::vec4 color = glm::vec4(1.0f);

        /** Color that the particle should have most of its lifetime (between fade in/out). */
        glm::vec4 targetColor = glm::vec4(1.0f);

        /** Current size. */
        float size = 0.25f;

        /** Size that the particle should have most of its lifetime (between fade in/out). */
        float targetSize = 0.25f;

        /** Time in seconds before destroyed. */
        float leftTimeToLive = 1.0f;

        /** Total time to live (constant). */
        float initialTimeToLive = 1.0f;
    };

    /**
     * Calculates random value according to the randomization where each component stores a non-negative value
     * that will be randomely added as [-value; +value].
     *
     * @param rnd   Random engine.
     * @param value Base value.
     * @param valueRandomization Randomization.
     *
     * @return Generated value.
     */
    static glm::vec3
    getValueWithRandomization(std::mt19937& rnd, const glm::vec3& value, const glm::vec3& valueRandomization);

    /** (Re)creates @ref pRenderingHandle. */
    void registerEmitterRendering();

    /** Data about currently active particles. */
    std::vector<ParticleData> vAliveParticleData;

    /** Not `nullptr` if being rendered. */
    std::unique_ptr<ParticleRenderingHandle> pRenderingHandle;

    /** Texture used by particles. */
    std::unique_ptr<TextureHandle> pTexture;

    /** Path (relative to the `res` directory) to the particle texture. */
    std::string sRelativePathToTexture;

    /** Velocity of spawned particles. */
    glm::vec3 spawnVelocity = glm::vec3(0.0f, 1.0f, 0.0f);

    /**
     * Each component stores a non-negative value that will be randomely added
     * to @ref velocity as [-value; +value].
     */
    glm::vec3 spawnVelocityRandomization = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Optional gravity to affect particles. */
    glm::vec3 gravity = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Color of a particle. */
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    /**
     * Each component stores a non-negative value that will be randomely added
     * to @ref color as [-value; +value].
     */
    glm::vec3 colorRandomization = glm::vec3(0.0f, 0.0f, 0.0f);

    /** Color that particle should have when fading in. */
    glm::vec4 colorFadeIn = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    /** Color that particle should have when fading out. */
    glm::vec4 colorFadeOut = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     */
    float fadeInLifePortion = 0.0f;

    /**
     * Value in range [0.0; 1.0] where 0 is time when particle is created and 1 is time when particle is
     * destroyed.
     */
    float fadeOutLifePortion = 1.0f;

    /** Size of a particle. */
    float size = 0.25f;

    /** Size of a particle when fading in. */
    float sizeFadeIn = 0.0f;

    /** Size of a particle when fading out. */
    float sizeFadeOut = 0.0f;

    /** The number of particles to spawn after each @ref delayBetweenSpawns. */
    unsigned int iParticleCountPerSpawn = 1;

    /** The maximum number of particles that will be randomly added to @ref iParticleCountPerSpawn. */
    unsigned int iParticleMaxAddCountPerSpawn = 0;

    /** Time in seconds between new batch of particles @ref iParticleCountPerSpawn is spawned. */
    float delayBetweenSpawns = 0.5f;

    /** The maximum time in seconds randomly added to @ref delayBetweenSpawns. */
    float delayBetweenSpawnsMaxAdd = 0.0f;

    /** Time in seconds before a new batch of particles should be spawned. */
    float timeBeforeParticleSpawn = 0.0f;

    /** Time in seconds before a particle is destroyed. */
    float timeToLive = 2.0f;

    /** The maximum time in seconds randomly added to @ref timeToLive. */
    float timeToLiveMaxAdd = 0.0f;

    /** `true` if simulation should be paused. */
    bool bIsPaused = false;
};
