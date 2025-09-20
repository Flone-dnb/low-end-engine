#include "game/node/physics/CompoundCollisionNode.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "24049922-c4ef-4c86-8fad-01c1bafab3ae";
}

std::string CompoundCollisionNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string CompoundCollisionNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo CompoundCollisionNode::getReflectionInfo() {
    ReflectedVariables variables;

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CompoundCollisionNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CompoundCollisionNode>(); },
        std::move(variables));
}

CompoundCollisionNode::CompoundCollisionNode() : CompoundCollisionNode("Compound Collision Node") {}
CompoundCollisionNode::CompoundCollisionNode(const std::string& sNodeName) : SpatialNode(sNodeName) {}

void CompoundCollisionNode::onChildNodesSpawned() {
    SpatialNode::onChildNodesSpawned();

    createPhysicsBody();

#if defined(DEBUG)
    iRecreateCompoundCount = 0;
#endif
}

void CompoundCollisionNode::onDespawning() {
    SpatialNode::onDespawning();

    destroyPhysicsBody();
}

void CompoundCollisionNode::createPhysicsBody() {
    if (pBody != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected physics body to not be created");
    }

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().createBodyForNode(this);
}

void CompoundCollisionNode::destroyPhysicsBody() {
    if (pBody == nullptr) {
        return;
    }

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
}

void CompoundCollisionNode::onChildCollisionChangedShape() {
    if (!isSpawned()) {
        return;
    }

    if (pBody == nullptr) {
        // Not created yet.
        return;
    }

    destroyPhysicsBody();
    createPhysicsBody();

#if defined(DEBUG) && !defined(ENGINE_EDITOR)
    iRecreateCompoundCount += 1;

    if (iRecreateCompoundCount >= 10) {
        iRecreateCompoundCount = 0;
        Logger::get().warn(std::format(
            "compound collision node \"{}\" was recreated multiple times since it was spawned due to changes "
            "in child collision nodes, changes to child nodes such as movement/rotation or shape change "
            "cause the whole compound to be recreated which might cause performance issues",
            getNodeName()));
    }
#endif
}

void CompoundCollisionNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (!isSpawned()) {
        return;
    }

    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setBodyLocationRotation(pBody, getWorldLocation(), getWorldRotation());
}

void CompoundCollisionNode::onAfterDirectChildDetached(Node* pDetachedDirectChild) {
    SpatialNode::onAfterDirectChildDetached(pDetachedDirectChild);

    if (!isSpawned()) {
        return;
    }

    destroyPhysicsBody();
    createPhysicsBody();
}

void CompoundCollisionNode::onAfterNewDirectChildAttached(Node* pNewDirectChild) {
    SpatialNode::onAfterNewDirectChildAttached(pNewDirectChild);

    if (!isSpawned()) {
        return;
    }

    destroyPhysicsBody();
    createPhysicsBody();
}
