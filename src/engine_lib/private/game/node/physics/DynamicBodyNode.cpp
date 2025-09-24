#include "game/node/physics/DynamicBodyNode.h"

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

std::string DynamicBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string DynamicBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo DynamicBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.bools[NAMEOF_MEMBER(&DynamicBodyNode::bIsSimulated).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<DynamicBodyNode*>(pThis)->setIsSimulated(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<DynamicBodyNode*>(pThis)->isSimulated();
        }};

    variables.floats[NAMEOF_MEMBER(&DynamicBodyNode::massKg).data()] = ReflectedVariableInfo<float>{
        .setter = [](Serializable* pThis,
                     const float& newValue) { reinterpret_cast<DynamicBodyNode*>(pThis)->setMass(newValue); },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<DynamicBodyNode*>(pThis)->getMass();
        }};

    variables.floats[NAMEOF_MEMBER(&DynamicBodyNode::friction).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<DynamicBodyNode*>(pThis)->setFriction(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<DynamicBodyNode*>(pThis)->getFriction();
        }};

    variables.floats[NAMEOF_MEMBER(&DynamicBodyNode::density).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<DynamicBodyNode*>(pThis)->setDensity(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<DynamicBodyNode*>(pThis)->getDensity();
        }};

    variables.serializables[NAMEOF_MEMBER(&DynamicBodyNode::pShape).data()] =
        ReflectedVariableInfo<std::unique_ptr<Serializable>>{
            .setter =
                [](Serializable* pThis, std::unique_ptr<Serializable> pNewValue) {
                    auto pNewShape =
                        std::unique_ptr<CollisionShape>(dynamic_cast<CollisionShape*>(pNewValue.release()));
                    if (pNewShape == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("invalid type for variable");
                    }
                    reinterpret_cast<DynamicBodyNode*>(pThis)->pShape = std::move(pNewShape);
                },
            .getter = [](Serializable* pThis) -> Serializable* {
                return reinterpret_cast<DynamicBodyNode*>(pThis)->pShape.get();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(DynamicBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<DynamicBodyNode>(); },
        std::move(variables));
}

DynamicBodyNode::DynamicBodyNode() : DynamicBodyNode("Dynamic Body Node") {}
DynamicBodyNode::DynamicBodyNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pShape = std::make_unique<BoxCollisionShape>();
    setOnShapeChangedCallback();
}

DynamicBodyNode::~DynamicBodyNode() {}

void DynamicBodyNode::recreateBodyIfSpawned() {
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
                    "physics body of the dynamic node \"{}\" was already recreated {} times after the node "
                    "was spawned, recreating the physics body often might cause performance issues, make "
                    "sure "
                    "you know what you're doing",
                    getNodeName(),
                    iBodyRecreateCountAfterSpawn));
                bWarnedAboutBodyRecreatingOften = true;
            }
        }
#endif
    }
}

void DynamicBodyNode::setOnShapeChangedCallback() {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    pShape->setOnChanged([this]() { recreateBodyIfSpawned(); });
}

void DynamicBodyNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    pShape = std::move(pNewShape);
    setOnShapeChangedCallback();
    recreateBodyIfSpawned();
}

void DynamicBodyNode::setDensity(float newDensity) {
    density = newDensity;
    recreateBodyIfSpawned();
}

void DynamicBodyNode::setMass(float newMassKg) {
    massKg = newMassKg;
    recreateBodyIfSpawned();
}

void DynamicBodyNode::setFriction(float newFriction) {
    friction = newFriction;
    recreateBodyIfSpawned();
}

void DynamicBodyNode::setIsSimulated(bool bActivate) {
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

void DynamicBodyNode::applyOneTimeImpulse(const glm::vec3& impulse) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addImpulseToBody(pBody, impulse);
}

void DynamicBodyNode::applyOneTimeAngularImpulse(const glm::vec3& impulse) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addAngularImpulseToBody(pBody, impulse);
}

void DynamicBodyNode::setForceForNextTick(const glm::vec3& force) {
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addForce(pBody, force);
}

void DynamicBodyNode::onSpawning() {
    SpatialNode::onSpawning();

#if defined(DEBUG)
    iBodyRecreateCountAfterSpawn = 0;
    bWarnedAboutBodyRecreatingOften = false;
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

void DynamicBodyNode::onDespawning() {
    SpatialNode::onDespawning();

    // Clear callback.
    pShape->setOnChanged([]() {});

    if (pBody != nullptr) {
        getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
    }
}

void DynamicBodyNode::setPhysicsSimulationResults(
    const glm::vec3& worldLocation, const glm::vec3& worldRotation) {
    bIsApplyingSimulationResults = true;

    setWorldLocation(worldLocation);
    setWorldRotation(worldRotation);

    bIsApplyingSimulationResults = false;
}

void DynamicBodyNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (bIsApplyingSimulationResults) {
#if defined(DEBUG) && !defined(ENGINE_EDITOR)
        if (!bWarnedAboutFallingOutOfWorld) {
            const auto worldLocation = getWorldLocation();
            if (worldLocation.z < -1000.0F) {
                Logger::get().warn(std::format(
                    "dynamic node \"{}\" seems to be falling out of the world, its world location is "
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
