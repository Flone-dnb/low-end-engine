#include "game/node/ParticleEmitterNode.h"

// Standard.
#include <glm/common.hpp>
#include <limits>

// Custom.
#include "render/RenderingHandle.h"
#include "render/ParticleRenderer.h"
#include "game/GameInstance.h"
#include "game/World.h"
#include "material/TextureManager.h"
#include "material/TextureHandle.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "0fa91b5e-1ab2-4f64-b482-f7a2531b962a";
    constexpr float minDelayBetweenSpawn = 0.01f;
    constexpr float minTimeToLive = 0.01f;
}

std::string ParticleEmitterNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string ParticleEmitterNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo ParticleEmitterNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.vec3s[NAMEOF_MEMBER(&ParticleEmitterNode::spawnVelocity).data()] =
        ReflectedVariableInfo<glm::vec3>{
            .setter =
                [](Serializable* pThis, const glm::vec3& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setSpawnVelocity(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec3 {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getSpawnVelocity();
            }};

    variables.vec3s[NAMEOF_MEMBER(&ParticleEmitterNode::spawnVelocityRandomization).data()] =
        ReflectedVariableInfo<glm::vec3>{
            .setter =
                [](Serializable* pThis, const glm::vec3& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setSpawnVelocityRandomization(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec3 {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getSpawnVelocityRandomization();
            }};

    variables.vec3s[NAMEOF_MEMBER(&ParticleEmitterNode::gravity).data()] = ReflectedVariableInfo<glm::vec3>{
        .setter =
            [](Serializable* pThis, const glm::vec3& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setGravity(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec3 {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getGravity();
        }};

    variables.vec4s[NAMEOF_MEMBER(&ParticleEmitterNode::color).data()] = ReflectedVariableInfo<glm::vec4>{
        .setter =
            [](Serializable* pThis, const glm::vec4& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setColor(newValue);
            },
        .getter = [](Serializable* pThis) -> glm::vec4 {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getColor();
        }};

    variables.vec3s[NAMEOF_MEMBER(&ParticleEmitterNode::colorRandomization).data()] =
        ReflectedVariableInfo<glm::vec3>{
            .setter =
                [](Serializable* pThis, const glm::vec3& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setColorRandomization(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec3 {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getColorRandomization();
            }};

    variables.vec4s[NAMEOF_MEMBER(&ParticleEmitterNode::colorFadeIn).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setColorFadeIn(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getColorFadeIn();
            }};

    variables.vec4s[NAMEOF_MEMBER(&ParticleEmitterNode::colorFadeOut).data()] =
        ReflectedVariableInfo<glm::vec4>{
            .setter =
                [](Serializable* pThis, const glm::vec4& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setColorFadeOut(newValue);
                },
            .getter = [](Serializable* pThis) -> glm::vec4 {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getColorFadeOut();
            }};

    variables.strings[NAMEOF_MEMBER(&ParticleEmitterNode::sRelativePathToTexture).data()] =
        ReflectedVariableInfo<std::string>{
            .setter =
                [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setRelativePathToTexture(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getRelativePathToTexture();
            }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::delayBetweenSpawns).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setDelayBetweenSpawns(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getDelayBetweenSpawns();
            }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::delayBetweenSpawnsMaxAdd).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setDelayBetweenSpawnsMaxAdd(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getDelayBetweenSpawnsMaxAdd();
            }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::fadeInLifePortion).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setFadeInLifePortion(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getFadeInLifePortion();
            }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::fadeOutLifePortion).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setFadeOutLifePortion(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getFadeOutLifePortion();
            }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::size).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setSize(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getSize();
        }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::sizeFadeIn).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setSizeFadeIn(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getSizeFadeIn();
        }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::sizeFadeOut).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setSizeFadeOut(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getSizeFadeOut();
        }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::timeToLive).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setTimeToLive(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->getTimeToLive();
        }};

    variables.floats[NAMEOF_MEMBER(&ParticleEmitterNode::timeToLiveMaxAdd).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setTimeToLiveMaxAdd(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getTimeToLiveMaxAdd();
            }};

    variables.unsignedInts[NAMEOF_MEMBER(&ParticleEmitterNode::iParticleCountPerSpawn).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter =
                [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setParticleCountPerSpawn(iNewValue);
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getParticleCountPerSpawn();
            }};

    variables.unsignedInts[NAMEOF_MEMBER(&ParticleEmitterNode::iParticleMaxAddCountPerSpawn).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter =
                [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<ParticleEmitterNode*>(pThis)->setParticleMaxAddCountPerSpawn(iNewValue);
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return reinterpret_cast<ParticleEmitterNode*>(pThis)->getParticleMaxAddCountPerSpawn();
            }};

    variables.bools[NAMEOF_MEMBER(&ParticleEmitterNode::bIsPaused).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<ParticleEmitterNode*>(pThis)->setIsPaused(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<ParticleEmitterNode*>(pThis)->isPaused();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(ParticleEmitterNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<ParticleEmitterNode>(); },
        std::move(variables));
}

ParticleEmitterNode::ParticleEmitterNode() : ParticleEmitterNode("Particle Emitter Node") {}
ParticleEmitterNode::ParticleEmitterNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    setIsCalledEveryFrame(true);
}

ParticleEmitterNode::~ParticleEmitterNode() {}

void ParticleEmitterNode::setParticleCountPerSpawn(unsigned int iParticleCount) {
    iParticleCountPerSpawn = iParticleCount;

    // Recreate because this affects max particle count (max GPU buffer size).
    if (pRenderingHandle != nullptr) {
        registerEmitterRendering();
    }
}

void ParticleEmitterNode::setParticleMaxAddCountPerSpawn(unsigned int iParticleCount) {
    iParticleMaxAddCountPerSpawn = iParticleCount;

    // Recreate because this affects max particle count (max GPU buffer size).
    if (pRenderingHandle != nullptr) {
        registerEmitterRendering();
    }
}

void ParticleEmitterNode::setTimeToLive(float time) {
    timeToLive = std::max(minTimeToLive, time);

    // Recreate because this affects max particle count (max GPU buffer size).
    if (pRenderingHandle != nullptr) {
        registerEmitterRendering();
    }
}

void ParticleEmitterNode::setDelayBetweenSpawns(float delay) {
    delayBetweenSpawns = std::max(minDelayBetweenSpawn, delay);

    // Recreate because this affects max particle count (max GPU buffer size).
    if (pRenderingHandle != nullptr) {
        registerEmitterRendering();
    }
}

void ParticleEmitterNode::setTimeToLiveMaxAdd(float time) {
    timeToLiveMaxAdd = std::max(0.0f, time);

    // Recreate because this affects max particle count (max GPU buffer size).
    if (pRenderingHandle != nullptr) {
        registerEmitterRendering();
    }
}

void ParticleEmitterNode::onSpawning() {
    SpatialNode::onSpawning();

    timeBeforeParticleSpawn = 0.0f;

    registerEmitterRendering();
}

void ParticleEmitterNode::onDespawning() {
    SpatialNode::onDespawning();

    vAliveParticleData.clear();
    pRenderingHandle = nullptr;
    pTexture = nullptr;
}

void ParticleEmitterNode::setRelativePathToTexture(std::string sNewRelativePathToTexture) {
    // Normalize slash.
    for (size_t i = 0; i < sNewRelativePathToTexture.size(); i++) {
        if (sNewRelativePathToTexture[i] == '\\') {
            sNewRelativePathToTexture[i] = '/';
        }
    }

    if (sRelativePathToTexture == sNewRelativePathToTexture) {
        return;
    }
    sRelativePathToTexture = sNewRelativePathToTexture;

    if (!sRelativePathToTexture.empty()) {
        // Make sure the path is valid.
        const auto pathToTexture =
            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sRelativePathToTexture;
        if (!std::filesystem::exists(pathToTexture)) {
            Log::error(std::format("path \"{}\" does not exist", pathToTexture.string()));
            return;
        }
        if (std::filesystem::is_directory(pathToTexture)) {
            Log::error(std::format("expected the path \"{}\" to point to a file", pathToTexture.string()));
            return;
        }
    }

    if (pRenderingHandle != nullptr) {
        auto guard =
            getWorldWhileSpawned()->getParticleRenderer().getParticleEmitterRenderData(*pRenderingHandle);
        auto& data = guard.getData();

        if (sRelativePathToTexture.empty()) {
            data.iTextureId = 0;
            pTexture = nullptr;
            return;
        }

        auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
            sRelativePathToTexture, TextureUsage::DIFFUSE);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));

        data.iTextureId = pTexture->getTextureId();
    }
}

void ParticleEmitterNode::registerEmitterRendering() {
    pRenderingHandle = nullptr;

    if (delayBetweenSpawns < minDelayBetweenSpawn || timeToLive < minTimeToLive) {
        return;
    }

    // Estimate max particle count to allocate a big enough GPU buffer.
    const size_t iMaxParticleCountPerSpawn = iParticleCountPerSpawn + iParticleMaxAddCountPerSpawn;
    const size_t iSpawnMultiplier =
        static_cast<size_t>(std::ceil((timeToLive + timeToLiveMaxAdd) / delayBetweenSpawns));

    // Check limit.
    size_t iMaxParticleCount = iMaxParticleCountPerSpawn * iSpawnMultiplier;
    if (iMaxParticleCount > std::numeric_limits<unsigned int>::max()) {
        iMaxParticleCount = std::numeric_limits<unsigned int>::max();
    }

    pRenderingHandle = getWorldWhileSpawned()->getParticleRenderer().registerParticleEmitter(
        static_cast<unsigned int>(iMaxParticleCount));

    // Initialize render data.
    auto guard =
        getWorldWhileSpawned()->getParticleRenderer().getParticleEmitterRenderData(*pRenderingHandle);
    auto& data = guard.getData();

    if (!sRelativePathToTexture.empty()) {
        auto result = getGameInstanceWhileSpawned()->getRenderer()->getTextureManager().getTexture(
            sRelativePathToTexture, TextureUsage::DIFFUSE);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            error.showErrorAndThrowException();
        }
        pTexture = std::get<std::unique_ptr<TextureHandle>>(std::move(result));

        data.iTextureId = pTexture->getTextureId();
    }

    data.vParticleData.clear();
}

void ParticleEmitterNode::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    SpatialNode::onBeforeNewFrame(timeSincePrevFrameInSec);

    if (bIsPaused) {
        return;
    }

    // Simulate existing particles.
    std::vector<ParticleData> vLeftParticles;
    vLeftParticles.reserve(vAliveParticleData.size());

    for (auto& particleData : vAliveParticleData) {
        particleData.leftTimeToLive -= timeSincePrevFrameInSec;
        if (particleData.leftTimeToLive <= 0.0f) {
            continue;
        }

        particleData.position += particleData.velocity * timeSincePrevFrameInSec;
        particleData.velocity += gravity * timeSincePrevFrameInSec;

        const float lifePortion = 1.0f - (particleData.leftTimeToLive / particleData.initialTimeToLive);
        const float fadeInPortion = 1.0f - glm::smoothstep(0.0f, fadeInLifePortion, lifePortion);
        const float fadeOutPortion = glm::smoothstep(fadeOutLifePortion, 1.0f, lifePortion);

        particleData.color = particleData.targetColor;
        particleData.color = glm::lerp(particleData.color, colorFadeIn, glm::vec4(fadeInPortion));
        particleData.color = glm::lerp(particleData.color, colorFadeOut, glm::vec4(fadeOutPortion));

        particleData.size = particleData.targetSize;
        particleData.size = glm::lerp(particleData.size, sizeFadeIn, fadeInPortion);
        particleData.size = glm::lerp(particleData.size, sizeFadeOut, fadeOutPortion);

        vLeftParticles.push_back(particleData);
    }

    vAliveParticleData = std::move(vLeftParticles);

    timeBeforeParticleSpawn -= timeSincePrevFrameInSec;
    if (timeBeforeParticleSpawn <= 0.0f) {
        // Spawn new particles.

        std::random_device randomDevice;
        std::mt19937 randomEngine(randomDevice());

        if (delayBetweenSpawnsMaxAdd == 0.0f) {
            timeBeforeParticleSpawn = delayBetweenSpawns;
        } else {
            std::uniform_real_distribution<float> delayDist(
                delayBetweenSpawns, delayBetweenSpawns + delayBetweenSpawnsMaxAdd);
            timeBeforeParticleSpawn = delayDist(randomEngine);
        }

        if (iParticleCountPerSpawn + iParticleMaxAddCountPerSpawn > 0) {
            unsigned int iNewParticleCount = 0;
            if (iParticleMaxAddCountPerSpawn == 0) {
                iNewParticleCount = iParticleCountPerSpawn;
            } else {
                std::uniform_int_distribution<unsigned long long> countDist(
                    iParticleCountPerSpawn, iParticleCountPerSpawn + iParticleMaxAddCountPerSpawn);

                const auto iNewCount = countDist(randomEngine);
                iNewParticleCount = iNewCount > std::numeric_limits<unsigned int>::max()
                                        ? std::numeric_limits<unsigned int>::max()
                                        : static_cast<unsigned int>(iNewCount);
            }

            vAliveParticleData.reserve(vAliveParticleData.size() + iNewParticleCount);
            ParticleData newParticleData{};
            newParticleData.position = getWorldLocation();

            std::uniform_real_distribution<float> lifeDist(timeToLive, timeToLive + timeToLiveMaxAdd);

            // Add new particles.
            for (unsigned int i = 0; i < iNewParticleCount; i++) {
                newParticleData.velocity =
                    getValueWithRandomization(randomEngine, spawnVelocity, spawnVelocityRandomization);
                newParticleData.targetColor = glm::vec4(
                    getValueWithRandomization(randomEngine, glm::vec3(color), colorRandomization), color.w);
                newParticleData.leftTimeToLive = lifeDist(randomEngine);
                newParticleData.initialTimeToLive = newParticleData.leftTimeToLive;
                newParticleData.targetSize = size;
                if (fadeInLifePortion > 0.0f) {
                    newParticleData.color = colorFadeIn;
                    newParticleData.size = sizeFadeIn;
                } else {
                    newParticleData.color = newParticleData.targetColor;
                    newParticleData.size = newParticleData.targetSize;
                }

                vAliveParticleData.push_back(newParticleData);
            }
        }
    }

    if (pRenderingHandle != nullptr) {
        // Update render data.
        auto guard =
            getWorldWhileSpawned()->getParticleRenderer().getParticleEmitterRenderData(*pRenderingHandle);
        auto& data = guard.getData();

        data.vParticleData.clear();
        data.vParticleData.resize(vAliveParticleData.size());

        for (size_t i = 0; i < vAliveParticleData.size(); i++) {
            auto& src = vAliveParticleData[i];
            auto& dst = data.vParticleData[i];

            dst.color = src.color;
            dst.positionAndSize = glm::vec4(src.position, src.size);
        }
    }
}

glm::vec3 ParticleEmitterNode::getValueWithRandomization(
    std::mt19937& rnd, const glm::vec3& value, const glm::vec3& valueRandomization) {
    std::uniform_real_distribution<float> xDist(
        value.x - valueRandomization.x, value.x + valueRandomization.x);
    std::uniform_real_distribution<float> yDist(
        value.y - valueRandomization.y, value.y + valueRandomization.y);
    std::uniform_real_distribution<float> zDist(
        value.z - valueRandomization.z, value.z + valueRandomization.z);

    return glm::vec3(xDist(rnd), yDist(rnd), zDist(rnd));
}
