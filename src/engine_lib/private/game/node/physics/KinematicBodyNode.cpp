#include "game/node/physics/KinematicBodyNode.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "68a03c5d-814d-4db4-aa91-a8ed51bb383d";
}

std::string KinematicBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string KinematicBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo KinematicBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    return TypeReflectionInfo(
        DynamicBodyNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(KinematicBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<KinematicBodyNode>(); },
        std::move(variables));
}

KinematicBodyNode::KinematicBodyNode() : KinematicBodyNode("Kinematic Body Node") {}
KinematicBodyNode::KinematicBodyNode(const std::string& sNodeName) : DynamicBodyNode(sNodeName) {}

KinematicBodyNode::~KinematicBodyNode() {}

void KinematicBodyNode::setLinearVelocity(const glm::vec3& velocity) {
    const auto pBody = getBody();
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setLinearVelocity(pBody, velocity);
}

void KinematicBodyNode::setAngularVelocity(const glm::vec3& velocity) {
    const auto pBody = getBody();
    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setAngularVelocity(pBody, velocity);
}

glm::vec3 KinematicBodyNode::getLinearVelocity() {
    const auto pBody = getBody();
    if (pBody == nullptr) {
        return glm::vec3(0.0F);
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getLinearVelocity(pBody);
}

glm::vec3 KinematicBodyNode::getAngularVelocity() {
    const auto pBody = getBody();
    if (pBody == nullptr) {
        return glm::vec3(0.0F);
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getAngularVelocity(pBody);
}
