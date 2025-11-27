#include "game/node/Sound3dNode.h"

// Custom.
#include "misc/ProjectPaths.h"
#include "game/GameInstance.h"
#include "game/GameManager.h"
#include "sound/SoundManager.h"
#include "game/Window.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "f27069de-6da6-4b3c-81cf-32cabf3d9191";
}

std::string Sound3dNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string Sound3dNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo Sound3dNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.strings[NAMEOF_MEMBER(&Sound3dNode::sPathToFileToPlay).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<Sound3dNode*>(pThis)->setPathToPlayRelativeRes(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<Sound3dNode*>(pThis)->getPathToPlayRelativeRes();
            }};

    variables.strings[NAMEOF_MEMBER(&Sound3dNode::soundChannel).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<Sound3dNode*>(pThis)->setSoundChannel(
                    convertSoundChannelNameToEnum(sNewValue));
            },
        .getter = [](Serializable* pThis) -> std::string {
            const auto optional = reinterpret_cast<Sound3dNode*>(pThis)->getSoundChannel();
            if (optional.has_value()) {
                return NAMEOF_ENUM(*optional).data();
            }
            return "";
        }};

    variables.floats[NAMEOF_MEMBER(&Sound3dNode::volume).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<Sound3dNode*>(pThis)->setVolume(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound3dNode*>(pThis)->getVolume();
        }};

    variables.floats[NAMEOF_MEMBER(&Sound3dNode::pitch).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<Sound3dNode*>(pThis)->setPitch(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound3dNode*>(pThis)->getPitch();
        }};

    variables.floats[NAMEOF_MEMBER(&Sound3dNode::maxVolumeDistance).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<Sound3dNode*>(pThis)->setMaxVolumeDistance(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound3dNode*>(pThis)->getMaxVolumeDistance();
        }};

    variables.floats[NAMEOF_MEMBER(&Sound3dNode::attenuation).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<Sound3dNode*>(pThis)->setAttenuation(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound3dNode*>(pThis)->getAttenuation();
        }};

    variables.bools[NAMEOF_MEMBER(&Sound3dNode::bAutoplayWhenSpawned).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<Sound3dNode*>(pThis)->setAutoplayWhenSpawned(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<Sound3dNode*>(pThis)->getAutoplayWhenSpawned();
        }};

    variables.bools[NAMEOF_MEMBER(&Sound3dNode::bIsLooping).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<Sound3dNode*>(pThis)->setIsLooping(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<Sound3dNode*>(pThis)->getIsLooping();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(Sound3dNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<Sound3dNode>(); },
        std::move(variables));
}

Sound3dNode::Sound3dNode() : Sound3dNode("Sound 3D Node") {}
Sound3dNode::Sound3dNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void Sound3dNode::setPathToPlayRelativeRes(std::string sPathToFile) {
    // Normalize slash.
    for (size_t i = 0; i < sPathToFile.size(); i++) {
        if (sPathToFile[i] == '\\') {
            sPathToFile[i] = '/';
        }
    }

    if (sPathToFileToPlay == sPathToFile) {
        return;
    }
    sPathToFileToPlay = sPathToFile;

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.stop();

    if (sPathToFileToPlay.empty()) {
        return;
    }

    loadAndPlay();
}

void Sound3dNode::setSoundChannel(SoundChannel channel) {
    if (isSpawned()) [[unlikely]] {
        // Sound manager does not expect this.
        Error::showErrorAndThrowException(std::format(
            "changing sound channel is not allowed while the node is spawned (node \"{}\")", getNodeName()));
    }

    soundChannel = channel;
}

void Sound3dNode::setVolume(float volume) {
    this->volume = std::max(0.0F, volume);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setVolume(volume * 100.0F); // NOLINT: SFML uses 0-100 volume
}

void Sound3dNode::setPitch(float pitch) {
    this->pitch = std::max(0.0F, pitch);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setPitch(pitch);
}

void Sound3dNode::setPlayingOffset(float seconds) {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setPlayingOffset(sf::seconds(seconds));
}

void Sound3dNode::setIsLooping(bool bEnableLooping) {
    bIsLooping = bEnableLooping;

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setLooping(bEnableLooping);
}

void Sound3dNode::setAutoplayWhenSpawned(bool bAutoplay) { this->bAutoplayWhenSpawned = bAutoplay; }

void Sound3dNode::setMaxVolumeDistance(float distance) {
    maxVolumeDistance = std::max(0.1F, distance); // NOLINT: 0.0 is an invalid value according to SFML docs

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setMinDistance(distance);
}

void Sound3dNode::setAttenuation(float attenuation) {
    this->attenuation = std::max(0.0F, attenuation);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setAttenuation(attenuation);
}

void Sound3dNode::playSound() {
    if (!isSpawned()) {
        return;
    }

    if (!bFileOpened) {
        if (!sfmlMusic.openFromFile(
                ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToFileToPlay))
            [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" failed to play sound from \"{}\" (is path correct?)",
                getNodeName(),
                sPathToFileToPlay));
        }
        bFileOpened = true;
    }

    sfmlMusic.play();
}

void Sound3dNode::pauseSound() {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.pause();
}

void Sound3dNode::stopSound() {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.stop();
}

float Sound3dNode::getDurationInSeconds() {
    if (sPathToFileToPlay.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("can't get sound duration - path to sound is not set (node \"{}\")", getNodeName()));
    }

    if (!bFileOpened) {
        if (!sfmlMusic.openFromFile(
                ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToFileToPlay))
            [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "node \"{}\" failed to play sound from \"{}\" (is path correct?)",
                getNodeName(),
                sPathToFileToPlay));
        }
        bFileOpened = true;
    }

    return sfmlMusic.getDuration().asSeconds();
}

void Sound3dNode::onSpawning() {
    SpatialNode::onSpawning();

    if (sPathToFileToPlay.empty()) {
        return;
    }

    // Notify manager.
    auto& soundManager = getGameInstanceWhileSpawned()->getWindow()->getGameManager()->getSoundManager();
    soundManager.onSoundNodeSpawned(this);

    loadAndPlay();
}

void Sound3dNode::loadAndPlay() {
    if (!sfmlMusic.openFromFile(
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToFileToPlay)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "node \"{}\" failed to play sound from \"{}\" (is path correct?)",
            getNodeName(),
            sPathToFileToPlay));
    }
    bFileOpened = true;

    sfmlMusic.setLooping(bIsLooping);
    sfmlMusic.setVolume(volume * 100.0F); // SFML uses 0-100 volume
    sfmlMusic.setPitch(pitch);

    sfmlMusic.setMinDistance(maxVolumeDistance);
    sfmlMusic.setAttenuation(attenuation);

    const auto pos = getWorldLocation();
    sfmlMusic.setPosition({pos.x, pos.y, pos.z});

    if (bAutoplayWhenSpawned) {
        sfmlMusic.play();
    }
}

void Sound3dNode::onDespawning() {
    SpatialNode::onDespawning();

    sfmlMusic.stop();

    // Notify manager.
    auto& soundManager = getGameInstanceWhileSpawned()->getWindow()->getGameManager()->getSoundManager();
    soundManager.onSoundNodeDespawned(this);

    bFileOpened = false;
    sfmlMusic = {};
}

void Sound3dNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (!isSpawned()) {
        return;
    }

    const auto pos = getWorldLocation();
    sfmlMusic.setPosition({pos.x, pos.y, pos.z});
}
