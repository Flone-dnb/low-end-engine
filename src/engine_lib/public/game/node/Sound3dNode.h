#pragma once

// Standard.
#include <string>
#include <optional>

// Custom.
#include "game/node/SpatialNode.h"
#include "sound/SoundChannel.hpp"

// External.
#include "SFML/Audio/Music.hpp"

/** Plays a 3D sound in world. */
class Sound3dNode : public SpatialNode {
public:
    Sound3dNode();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    Sound3dNode(const std::string& sNodeName);

    virtual ~Sound3dNode() override = default;

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
     * Sets path to an audio file to play. If the node is not spawned yet this file will be played when
     * spawned, otherwise if the node is already spawned it will start playing the sound right away.
     *
     * @param sPathToFile Path relative to the `res` directory.
     */
    void setPathToPlayRelativeRes(std::string sPathToFile);

    /**
     * Sets category of the sound.
     *
     * @param channel Sound category.
     */
    void setSoundChannel(SoundChannel channel);

    /**
     * Sets sound volume multiplier.
     *
     * @param volume Positive number where 1.0 means no changes in sound and 0.0 means no sound (mute).
     */
    void setVolume(float volume);

    /**
     * Sets the pitch of the sound.
     *
     * @param pitch Positive number where 1.0 means no changes in pitch.
     */
    void setPitch(float pitch);

    /**
     * If sound is playing changes the currently playing position.
     *
     * @param seconds Offset in seconds (from the beginning of the sound).
     */
    void setPlayingOffset(float seconds);

    /**
     * Sets whether to restart the sound after it ended or not.
     *
     * @param bEnableLooping `true` to enable.
     */
    void setIsLooping(bool bEnableLooping);

    /**
     * Sets whether or not the sound should play right after the node is spawned.
     *
     * @param bAutoplay Autoplay state.
     */
    void setAutoplayWhenSpawned(bool bAutoplay);

    /**
     * Sets distance under which the sound will be heard at its maximum volume.
     *
     * @param distance Value in range [0.1; +inf].
     */
    void setMaxVolumeDistance(float distance);

    /**
     * Sets sound attenuation. The greater the attenuation, the less it will be heard when the sound moves
     * away from the listener.
     *
     * @param attenuation Value in range [0.1; +inf].
     */
    void setAttenuation(float attenuation);

    /**
     * Plays the sound from @ref getPathToPlayRelativeRes (continues if was paused).
     *
     * @remark Does nothing if not spawned or if @ref getPathToPlayRelativeRes is empty.
     */
    void playSound();

    /** Pauses the currently playing sound (if playing). */
    void pauseSound();

    /** Stops the currently playing sound (if playing). */
    void stopSound();

    /**
     * Returns path to a file (relative to the `res` directory) to play when/while spawned.
     *
     * @return Empty if nothing to play.
     */
    std::string getPathToPlayRelativeRes() const { return sPathToFileToPlay; }

    /**
     * Returns sound channel that the sound uses.
     *
     * @return Empty if no sound was assigned yet.
     */
    std::optional<SoundChannel> getSoundChannel() const { return soundChannel; }

    /**
     * Returns volume multiplier of the sound.
     *
     * @return Positive number where 1.0 means no changes in sound and 0.0 means no sound (mute).
     */
    float getVolume() const { return volume; }

    /**
     * Returns the pitch of the sound.
     *
     * @return Positive number where 1.0 means no changes in pitch.
     */
    float getPitch() const { return pitch; }

    /**
     * Returns duration of the sound from @ref getPathToPlayRelativeRes.
     *
     * @return Duration in seconds.
     */
    float getDurationInSeconds();

    /**
     * Returns distance under which the sound will be heard at its maximum volume.
     *
     * @return Value in range [0.1; +inf].
     */
    float getMaxVolumeDistance() const { return maxVolumeDistance; }

    /**
     * Returns sound attenuation. The greater the attenuation, the less it will be heard when the sound moves
     * away from the listener.
     *
     * @return Value in range [0.0; +inf].
     */
    float getAttenuation() const { return attenuation; }

    /**
     * Tells whether to restart the sound after it ended or not.
     *
     * @return Looping state.
     */
    bool getIsLooping() const { return bIsLooping; }

    /**
     * If @ref getPathToPlayRelativeRes is specified autoplays the sound when spawned.
     *
     * @return Autoplay state.
     */
    bool getAutoplayWhenSpawned() const { return bAutoplayWhenSpawned; }

protected:
    /**
     * Called when this node was not spawned previously and it was either attached to a parent node
     * that is spawned or set as world's root node.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as spawned before this function is called.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     * @remark This function is called before any of the child nodes are spawned. If you
     * need to do some logic after child nodes are spawned use @ref onChildNodesSpawned.
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
     * Called after node's world location/rotation/scale was changed.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     */
    virtual void onWorldLocationRotationScaleChanged() override;

private:
    /** Thing that plays sound. */
    sf::Music sfmlMusic;

    /** Empty if nothing to play. Path (relative to the `res` directory) to play when spawned. */
    std::string sPathToFileToPlay;

    /** Mixer channel. */
    std::optional<SoundChannel> soundChannel;

    /** Distance under which the sound will be heard at its maximum volume, range [0.1; +inf]. */
    float maxVolumeDistance = 1.0F; // NOLINT

    /** The greater the attenuation, the less it will be heard when the sound moves away from the listener. */
    float attenuation = 2.0F; // NOLINT

    /** Sound volume multiplier, positive number. */
    float volume = 1.0F;

    /** Pitch of the sound, positive number. */
    float pitch = 1.0F;

    /** Whether to restart the sound after it ended or not. */
    bool bIsLooping = false;

    /** If @ref sPathToFileToPlay is specified autoplays the sound when spawned. */
    bool bAutoplayWhenSpawned = false;

    /** `true` if @ref sPathToFileToPlay was opened to play. */
    bool bFileOpened = false;
};
