#include "game/node/Sound2dNode.h"

// Custom.
#include "misc/ProjectPaths.h"
#include "game/GameInstance.h"
#include "game/GameManager.h"
#include "sound/SoundManager.h"
#include "game/Window.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "08584676-9814-4cd2-95bf-d956573057e9";
}

std::string Sound2dNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string Sound2dNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo Sound2dNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.strings[NAMEOF_MEMBER(&Sound2dNode::sPathToFileToPlay).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<Sound2dNode*>(pThis)->setPathToPlayRelativeRes(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<Sound2dNode*>(pThis)->getPathToPlayRelativeRes();
            }};

    variables.strings[NAMEOF_MEMBER(&Sound2dNode::soundChannel).data()] = ReflectedVariableInfo<std::string>{
        .setter =
            [](Serializable* pThis, const std::string& sNewValue) {
                reinterpret_cast<Sound2dNode*>(pThis)->setSoundChannel(
                    convertSoundChannelNameToEnum(sNewValue));
            },
        .getter = [](Serializable* pThis) -> std::string {
            const auto optional = reinterpret_cast<Sound2dNode*>(pThis)->getSoundChannel();
            if (optional.has_value()) {
                return NAMEOF_ENUM(*optional).data();
            }
            return "";
        }};

    variables.floats[NAMEOF_MEMBER(&Sound2dNode::volume).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<Sound2dNode*>(pThis)->setVolume(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound2dNode*>(pThis)->getVolume();
        }};

    variables.floats[NAMEOF_MEMBER(&Sound2dNode::pitch).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<Sound2dNode*>(pThis)->setPitch(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound2dNode*>(pThis)->getPitch();
        }};

    variables.floats[NAMEOF_MEMBER(&Sound2dNode::pan).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<Sound2dNode*>(pThis)->setPan(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<Sound2dNode*>(pThis)->getPan();
        }};

    variables.bools[NAMEOF_MEMBER(&Sound2dNode::bAutoplayWhenSpawned).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<Sound2dNode*>(pThis)->setAutoplayWhenSpawned(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<Sound2dNode*>(pThis)->getAutoplayWhenSpawned();
        }};

    variables.bools[NAMEOF_MEMBER(&Sound2dNode::bIsLooping).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<Sound2dNode*>(pThis)->setIsLooping(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<Sound2dNode*>(pThis)->getIsLooping();
        }};

    return TypeReflectionInfo(
        Node::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(Sound2dNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<Sound2dNode>(); },
        std::move(variables));
}

Sound2dNode::Sound2dNode() : Sound2dNode("Sound 2D Node") {}
Sound2dNode::Sound2dNode(const std::string& sNodeName) : Node(sNodeName) {}

void Sound2dNode::setPathToPlayRelativeRes(std::string sPathToFile) {
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
}

void Sound2dNode::setSoundChannel(SoundChannel channel) {
    if (isSpawned()) [[unlikely]] {
        // Sound manager does not expect this.
        Error::showErrorAndThrowException(std::format(
            "changing sound channel is not allowed while the node is spawned (node \"{}\")", getNodeName()));
    }

    soundChannel = channel;
}

void Sound2dNode::setVolume(float volume) {
    this->volume = std::max(0.0F, volume);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setVolume(volume * 100.0F); // NOLINT: SFML uses 0-100 volume
}

void Sound2dNode::setPitch(float pitch) {
    this->pitch = std::max(0.0F, pitch);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setPitch(pitch);
}

void Sound2dNode::setPan(float pan) {
    this->pan = std::clamp(pan, -1.0F, 1.0F);

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setPan(pan);
}

void Sound2dNode::setPlayingOffset(float seconds) {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setPlayingOffset(sf::seconds(seconds));
}

void Sound2dNode::setIsLooping(bool bEnableLooping) {
    bIsLooping = bEnableLooping;

    if (!isSpawned()) {
        return;
    }

    sfmlMusic.setLooping(bEnableLooping);
}

void Sound2dNode::setAutoplayWhenSpawned(bool bAutoplay) { this->bAutoplayWhenSpawned = bAutoplay; }

void Sound2dNode::playSound() {
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

void Sound2dNode::pauseSound() {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.pause();
}

void Sound2dNode::stopSound() {
    if (!isSpawned()) {
        return;
    }

    sfmlMusic.stop();
}

float Sound2dNode::getDurationInSeconds() {
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

void Sound2dNode::onSpawning() {
    Node::onSpawning();

    if (sPathToFileToPlay.empty()) {
        return;
    }

    if (!sfmlMusic.openFromFile(
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToFileToPlay)) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "node \"{}\" failed to play sound from \"{}\" (is path correct?)",
            getNodeName(),
            sPathToFileToPlay));
    }
    bFileOpened = true;

    // Notify manager.
    auto& soundManager = getGameInstanceWhileSpawned()->getWindow()->getGameManager()->getSoundManager();
    soundManager.onSoundNodeSpawned(this);

    sfmlMusic.setLooping(bIsLooping);
    sfmlMusic.setVolume(volume * 100.0F); // NOLINT: SFML uses 0-100 volume
    sfmlMusic.setPitch(pitch);
    sfmlMusic.setPan(pan);

    if (bAutoplayWhenSpawned) {
        sfmlMusic.play();
    }
}

void Sound2dNode::onDespawning() {
    Node::onDespawning();

    sfmlMusic.stop();

    // Notify manager.
    auto& soundManager = getGameInstanceWhileSpawned()->getWindow()->getGameManager()->getSoundManager();
    soundManager.onSoundNodeDespawned(this);

    bFileOpened = false;
    sfmlMusic = {};
}
