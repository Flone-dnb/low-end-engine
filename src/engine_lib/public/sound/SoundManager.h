#pragma once

// Standard.
#include <mutex>
#include <array>
#include <unordered_set>

// Custom.
#include "sound/SoundChannel.hpp"

class Sound2dNode;
class Sound3dNode;
class CameraManager;

/** Keeps track of all spawned sound nodes and handles sound effect management. */
class SoundManager {
    // Only game manager is expected to create objects of this type.
    friend class GameManager;

    // Nodes notify the manager when they spawn/despawn.
    friend class Sound2dNode;
    friend class Sound3dNode;

public:
    ~SoundManager();

    /**
     * Sets sound volume (for all sounds).
     *
     * @param volume Value in range [0.0, 2.0] where 0.0 means silence and 1.0 means 100% volume.
     */
    static void setSoundVolume(float volume);

private:
    /** Groups spawned sound nodes. */
    struct SpawnedSoundNodes {
        /** Sound 2D nodes. */
        std::array<std::unordered_set<Sound2dNode*>, static_cast<size_t>(SoundChannel::COUNT)>
            vSound2dNodesByChannel;

        /** Sound 3D nodes. */
        std::array<std::unordered_set<Sound3dNode*>, static_cast<size_t>(SoundChannel::COUNT)>
            vSound3dNodesByChannel;
    };

    SoundManager() = default;

    /**
     * Called to update listener's direction and position.
     *
     * @param pCameraManager Camera manager.
     */
    static void onBeforeNewFrame(CameraManager* pCameraManager);

    /**
     * Called by sound nodes when they spawn.
     *
     * @param pNode Sound node.
     */
    void onSoundNodeSpawned(Sound2dNode* pNode);

    /**
     * Called by sound nodes when they despawn.
     *
     * @param pNode Sound node.
     */
    void onSoundNodeDespawned(Sound2dNode* pNode);

    /**
     * Called by sound nodes when they spawn.
     *
     * @param pNode Sound node.
     */
    void onSoundNodeSpawned(Sound3dNode* pNode);

    /**
     * Called by sound nodes when they despawn.
     *
     * @param pNode Sound node.
     */
    void onSoundNodeDespawned(Sound3dNode* pNode);

    /** All currently spawned sound nodes. */
    std::pair<std::mutex, SpawnedSoundNodes> mtxSpawnedNodes;
};
