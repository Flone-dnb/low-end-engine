#include "sound/SoundManager.h"

// Standard.
#include <format>

// Custom.
#include "game/node/Sound2dNode.h"
#include "game/node/Sound3dNode.h"
#include "game/camera/CameraManager.h"
#include "game/node/CameraNode.h"
#include "misc/Error.h"

// SFML.
#include "SFML/Audio/Listener.hpp"

SoundManager::~SoundManager() {
    std::scoped_lock guard(mtxSpawnedNodes.first);

    for (const auto& nodes : mtxSpawnedNodes.second.vSound2dNodesByChannel) {
        if (!nodes.empty()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "sound manager is being destroyed but there are still {} sound node(s)", nodes.size()));
        }
    }

    for (const auto& nodes : mtxSpawnedNodes.second.vSound3dNodesByChannel) {
        if (!nodes.empty()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "sound manager is being destroyed but there are still {} sound node(s)", nodes.size()));
        }
    }
}

void SoundManager::setSoundVolume(float volume) {
    sf::Listener::setGlobalVolume(std::clamp(volume, 0.0F, 2.0F) * 100.0F); // NOLINT
}

void SoundManager::onBeforeNewFrame(CameraManager* pCameraManager) {
    auto& mtxActiveCamera = pCameraManager->getActiveCamera();
    std::scoped_lock guard(mtxActiveCamera.first);

    if (mtxActiveCamera.second == nullptr) {
        return;
    }

    const auto pos = mtxActiveCamera.second->getWorldLocation();
    const auto forward = mtxActiveCamera.second->getWorldForwardDirection();
    const auto up = mtxActiveCamera.second->getWorldUpDirection(); // NOLINT

    sf::Listener::setPosition({pos.x, pos.y, pos.z});
    sf::Listener::setDirection({forward.x, forward.y, forward.z});
    sf::Listener::setUpVector({up.x, up.y, -up.z});
}

void SoundManager::onSoundNodeSpawned(Sound2dNode* pNode) {
    // Get sound channel.
    const auto optionalSoundChannel = pNode->getSoundChannel();
    if (!optionalSoundChannel.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" must have a sound channel specified", pNode->getNodeName()));
    }

    if (*optionalSoundChannel == SoundChannel::COUNT) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" has invalid sound channel", pNode->getNodeName()));
    }

    // Register.
    if (!mtxSpawnedNodes.second.vSound2dNodesByChannel[static_cast<size_t>(*optionalSoundChannel)]
             .insert(pNode)
             .second) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "sound node \"{}\" is already registered in the sound manager", pNode->getNodeName()));
    }
}

void SoundManager::onSoundNodeDespawned(Sound2dNode* pNode) {
    // Get sound channel.
    const auto optionalSoundChannel = pNode->getSoundChannel();
    if (!optionalSoundChannel.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" must have a sound channel specified", pNode->getNodeName()));
    }

    if (*optionalSoundChannel == SoundChannel::COUNT) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" has invalid sound channel", pNode->getNodeName()));
    }

    // Unregister.
    auto& nodes = mtxSpawnedNodes.second.vSound2dNodesByChannel[static_cast<size_t>(*optionalSoundChannel)];
    const auto it = nodes.find(pNode);
    if (it == nodes.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" is not registered in the sound manager", pNode->getNodeName()));
    }
    nodes.erase(it);
}

void SoundManager::onSoundNodeSpawned(Sound3dNode* pNode) {
    // Get sound channel.
    const auto optionalSoundChannel = pNode->getSoundChannel();
    if (!optionalSoundChannel.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" must have a sound channel specified", pNode->getNodeName()));
    }

    if (*optionalSoundChannel == SoundChannel::COUNT) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" has invalid sound channel", pNode->getNodeName()));
    }

    // Register.
    if (!mtxSpawnedNodes.second.vSound3dNodesByChannel[static_cast<size_t>(*optionalSoundChannel)]
             .insert(pNode)
             .second) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "sound node \"{}\" is already registered in the sound manager", pNode->getNodeName()));
    }
}

void SoundManager::onSoundNodeDespawned(Sound3dNode* pNode) {
    // Get sound channel.
    const auto optionalSoundChannel = pNode->getSoundChannel();
    if (!optionalSoundChannel.has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" must have a sound channel specified", pNode->getNodeName()));
    }

    if (*optionalSoundChannel == SoundChannel::COUNT) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" has invalid sound channel", pNode->getNodeName()));
    }

    // Unregister.
    auto& nodes = mtxSpawnedNodes.second.vSound3dNodesByChannel[static_cast<size_t>(*optionalSoundChannel)];
    const auto it = nodes.find(pNode);
    if (it == nodes.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" is not registered in the sound manager", pNode->getNodeName()));
    }
    nodes.erase(it);
}
