#include "game/node/physics/SimulatedBodyNode.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/geometry/shapes/CollisionShape.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "a7c3445a-edfd-40ad-864d-8146309d17b6";
}

std::string SimulatedBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SimulatedBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SimulatedBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.bools[NAMEOF_MEMBER(&SimulatedBodyNode::bIsSimulated).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<SimulatedBodyNode*>(pThis)->setIsSimulated(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<SimulatedBodyNode*>(pThis)->isSimulated();
        }};

    variables.floats[NAMEOF_MEMBER(&SimulatedBodyNode::massKg).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<SimulatedBodyNode*>(pThis)->setMass(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SimulatedBodyNode*>(pThis)->getMass();
        }};

    variables.floats[NAMEOF_MEMBER(&SimulatedBodyNode::friction).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SimulatedBodyNode*>(pThis)->setFriction(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SimulatedBodyNode*>(pThis)->getFriction();
        }};

    variables.floats[NAMEOF_MEMBER(&SimulatedBodyNode::density).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<SimulatedBodyNode*>(pThis)->setDensity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<SimulatedBodyNode*>(pThis)->getDensity();
        }};

    variables.serializables[NAMEOF_MEMBER(&SimulatedBodyNode::pShape).data()] =
        ReflectedVariableInfo<std::unique_ptr<Serializable>>{
            .setter =
                [](Serializable* pThis, std::unique_ptr<Serializable> pNewValue) {
                    auto pNewShape =
                        std::unique_ptr<CollisionShape>(dynamic_cast<CollisionShape*>(pNewValue.release()));
                    if (pNewShape == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("invalid type for variable");
                    }
                    reinterpret_cast<SimulatedBodyNode*>(pThis)->pShape = std::move(pNewShape);
                },
            .getter = [](Serializable* pThis) -> Serializable* {
                return reinterpret_cast<SimulatedBodyNode*>(pThis)->pShape.get();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SimulatedBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SimulatedBodyNode>(); },
        std::move(variables));
}

SimulatedBodyNode::SimulatedBodyNode() : SimulatedBodyNode("Simulated Body Node") {}
SimulatedBodyNode::SimulatedBodyNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pShape = std::make_unique<BoxCollisionShape>();
    setOnShapeChangedCallback();
}

SimulatedBodyNode::~SimulatedBodyNode() {}

void SimulatedBodyNode::recreateBodyIfSpawned() {
    if (!isSpawned()) {
        return;
    }

    if (pBody != nullptr) {
        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);

        if (bIsSimulated) {
#if !defined(ENGINE_EDITOR)
            physicsManager.setBodyActiveState(pBody, true);
#endif
        }

#if defined(DEBUG) && !defined(ENGINE_EDITOR)
        if (!bWarnedAboutBodyRecreatingOften) {
            iBodyRecreateCountAfterSpawn += 1;
            if (iBodyRecreateCountAfterSpawn >= 10) {
                Logger::get().warn(std::format(
                    "physics body of the simulated node \"{}\" was already recreated {} times after the node "
                    "was spawned, recreating the physics body often might cause performance issues, make "
                    "sure you know what you're doing",
                    getNodeName(),
                    iBodyRecreateCountAfterSpawn));
                bWarnedAboutBodyRecreatingOften = true;
            }
        }
#endif
    }
}

void SimulatedBodyNode::setOnShapeChangedCallback() {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    pShape->setOnChanged([this]() { recreateBodyIfSpawned(); });
}

void SimulatedBodyNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    if (pNewShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("`nullptr` is not a valid shape (node \"{}\")", getNodeName()));
    }

    pShape = std::move(pNewShape);
    setOnShapeChangedCallback();
    recreateBodyIfSpawned();
}

void SimulatedBodyNode::setDensity(float newDensity) {
    density = newDensity;
    recreateBodyIfSpawned();
}

void SimulatedBodyNode::setMass(float newMassKg) {
    massKg = newMassKg;
    recreateBodyIfSpawned();
}

void SimulatedBodyNode::setFriction(float newFriction) {
    friction = newFriction;
    recreateBodyIfSpawned();
}

void SimulatedBodyNode::setIsSimulated(bool bActivate) {
    if (bIsSimulated == bActivate) {
        return;
    }
    bIsSimulated = bActivate;

    if (!isSpawned()) {
        return;
    }

    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();

    if (bIsSimulated) {
#if !defined(ENGINE_EDITOR)
        physicsManager.setBodyActiveState(pBody, true);
#endif
    } else {
        physicsManager.setBodyActiveState(pBody, false);
    }
}

void SimulatedBodyNode::applyOneTimeImpulse(const glm::vec3& impulse) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addImpulseToBody(pBody, impulse);
}

void SimulatedBodyNode::applyOneTimeAngularImpulse(const glm::vec3& impulse) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addAngularImpulseToBody(pBody, impulse);
}

void SimulatedBodyNode::setForceForNextTick(const glm::vec3& force) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addForce(pBody, force);
}

CollisionShape& SimulatedBodyNode::getShape() const {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("simulated body node \"{}\" has invalid shape", getNodeName()));
    }

    return *pShape;
}

void SimulatedBodyNode::onSpawning() {
    SpatialNode::onSpawning();

#if defined(DEBUG)
    iBodyRecreateCountAfterSpawn = 0;
    bWarnedAboutBodyRecreatingOften = false;
    bWarnedAboutFallingOutOfWorld = false;
#endif

    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected collision node \"{}\" to have a valid shape when spawning", getNodeName()));
    }
    setOnShapeChangedCallback();

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.createBodyForNode(this);

#if !defined(ENGINE_EDITOR)
    if (bIsSimulated) {
        physicsManager.setBodyActiveState(pBody, true);
    }
#endif
}

void SimulatedBodyNode::onDespawning() {
    SpatialNode::onDespawning();

    // Clear callback.
    pShape->setOnChanged([]() {});

    if (pBody != nullptr) {
        getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
    }
}

void SimulatedBodyNode::setPhysicsSimulationResults(
    const glm::vec3& worldLocation, const glm::vec3& worldRotation) {
    bIsApplyingSimulationResults = true;

    setWorldLocation(worldLocation);
    setWorldRotation(worldRotation);

    bIsApplyingSimulationResults = false;
}

void SimulatedBodyNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (bIsApplyingSimulationResults) {
#if defined(DEBUG)
        if (!bWarnedAboutFallingOutOfWorld) {
            const auto worldLocation = getWorldLocation();
            if (worldLocation.z < -1000.0F) {
                Logger::get().warn(std::format(
                    "simulated node \"{}\" seems to be falling out of the world, its world location is "
                    "({}, {}, {})",
                    getNodeName(),
                    worldLocation.x,
                    worldLocation.y,
                    worldLocation.z));
                bWarnedAboutFallingOutOfWorld = true;
            }
        }
#endif
        return;
    }

    if (!isSpawned() || pBody == nullptr) {
        // Not created yet.
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setBodyLocationRotation(pBody, getWorldLocation(), getWorldRotation());
}

glm::vec3 SimulatedBodyNode::getGravityWhileSpawned() {
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getGravity();
}