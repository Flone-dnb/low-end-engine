#include "game/node/physics/MovingBodyNode.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/geometry/shapes/CollisionShape.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "68a03c5d-814d-4db4-aa91-a8ed51bb383d";
}

std::string MovingBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string MovingBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo MovingBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(MovingBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<MovingBodyNode>(); },
        std::move(variables));
}

MovingBodyNode::MovingBodyNode() : MovingBodyNode("Moving Body Node") {}
MovingBodyNode::MovingBodyNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pShape = std::make_unique<BoxCollisionShape>();
    setOnShapeChangedCallback();
}

MovingBodyNode::~MovingBodyNode() {}

void MovingBodyNode::setVelocityToBeAt(
    const glm::vec3& worldLocation, const glm::vec3& worldRotation, float deltaTime) {
    if (pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

#if defined(DEBUG)
    if (!bIsInPhysicsTick) [[unlikely]] {
        Error::showErrorAndThrowException(
            "this and similar physics functions should be called in onBeforePhysicsUpdate");
    }
#endif

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.moveKinematic(pBody, worldLocation, worldRotation, deltaTime);
}

void MovingBodyNode::setLinearVelocity(const glm::vec3& velocity) {
    if (pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

#if defined(DEBUG)
    if (!bIsInPhysicsTick) [[unlikely]] {
        Error::showErrorAndThrowException(
            "this and similar physics functions should be called in onBeforePhysicsUpdate");
    }
#endif

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setLinearVelocity(pBody, velocity);
}

void MovingBodyNode::setAngularVelocity(const glm::vec3& velocity) {
    if (pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

#if defined(DEBUG)
    if (!bIsInPhysicsTick) [[unlikely]] {
        Error::showErrorAndThrowException(
            "this and similar physics functions should be called in onBeforePhysicsUpdate");
    }
#endif

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setAngularVelocity(pBody, velocity);
}

glm::vec3 MovingBodyNode::getLinearVelocity() {
    if (pBody == nullptr) {
        return glm::vec3(0.0F);
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getLinearVelocity(pBody);
}

glm::vec3 MovingBodyNode::getAngularVelocity() {
    if (pBody == nullptr) {
        return glm::vec3(0.0F);
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getAngularVelocity(pBody);
}

CollisionShape& MovingBodyNode::getShape() const {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("dynamic body node \"{}\" has invalid shape", getNodeName()));
    }

    return *pShape;
}

void MovingBodyNode::onSpawning() {
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
}

void MovingBodyNode::onDespawning() {
    SpatialNode::onDespawning();

    // Clear callback.
    pShape->setOnChanged([]() {});

    if (pBody != nullptr) {
        getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
    }
}

void MovingBodyNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (bIsApplyingSimulationResults) {
#if defined(DEBUG)
        if (!bWarnedAboutFallingOutOfWorld) {
            const auto worldLocation = getWorldLocation();
            if (worldLocation.y < -1000.0F) {
                Logger::get().warn(std::format(
                    "moving node \"{}\" seems to be falling out of the world, its world location is "
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

glm::vec3 MovingBodyNode::getGravityWhileSpawned() {
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getGravity();
}

void MovingBodyNode::setPhysicsSimulationResults(
    const glm::vec3& worldLocation, const glm::vec3& worldRotation) {
    bIsApplyingSimulationResults = true;

    setWorldLocation(worldLocation);
    setWorldRotation(worldRotation);

    bIsApplyingSimulationResults = false;
}

void MovingBodyNode::setOnShapeChangedCallback() {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    pShape->setOnChanged([this]() { recreateBodyIfSpawned(); });
}

void MovingBodyNode::recreateBodyIfSpawned() {
    if (!isSpawned()) {
        return;
    }

    if (pBody != nullptr) {
        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);

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
