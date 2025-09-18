#include "game/node/physics/CollisionNode.h"

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/geometry/shapes/CollisionShape.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "9dca5a60-69a8-4ef0-93f4-1ba2786cdd76";
}

std::string CollisionNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string CollisionNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo CollisionNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.serializables[NAMEOF_MEMBER(&CollisionNode::pShape).data()] =
        ReflectedVariableInfo<std::unique_ptr<Serializable>>{
            .setter =
                [](Serializable* pThis, std::unique_ptr<Serializable> pNewValue) {
                    auto pNewShape =
                        std::unique_ptr<CollisionShape>(dynamic_cast<CollisionShape*>(pNewValue.release()));
                    if (pNewShape == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("invalid type for variable");
                    }
                    reinterpret_cast<CollisionNode*>(pThis)->pShape = std::move(pNewShape);
                },
            .getter = [](Serializable* pThis) -> Serializable* {
                return reinterpret_cast<CollisionNode*>(pThis)->pShape.get();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CollisionNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CollisionNode>(); },
        std::move(variables));
}

CollisionNode::CollisionNode() : CollisionNode("Collision Node") {}
CollisionNode::CollisionNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pShape = std::make_unique<BoxCollisionShape>();
}

void CollisionNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    pShape = std::move(pNewShape);
    pShape->setOnChanged([this]() {
        if (isSpawned()) {
            auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
            physicsManager.destroyBodyForNode(this);
            physicsManager.createBodyForNode(this);
        }
    });

    if (isSpawned()) {
        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);
    }
}

void CollisionNode::onSpawning() {
    SpatialNode::onSpawning();

    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected collision node \"{}\" to have a valid shape when spawning", getNodeName()));
    }

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().createBodyForNode(this);

    pShape->setOnChanged([this]() {
        if (isSpawned()) {
            auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
            physicsManager.destroyBodyForNode(this);
            physicsManager.createBodyForNode(this);
        }
    });
}

void CollisionNode::onDespawning() {
    SpatialNode::onDespawning();

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);

    // Clear callback.
    pShape->setOnChanged([]() {});
}

void CollisionNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setBodyLocationRotation(pBody, getWorldLocation(), getWorldRotation());
}
