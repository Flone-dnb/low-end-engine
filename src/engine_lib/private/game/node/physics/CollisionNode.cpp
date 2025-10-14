#include "game/node/physics/CollisionNode.h"

// Standard.
#include <format>

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/geometry/shapes/CollisionShape.h"
#include "game/node/physics/CompoundCollisionNode.h"

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

    variables.bools[NAMEOF_MEMBER(&CollisionNode::bIsCollisionEnabled).data()] = ReflectedVariableInfo<bool>{
        .setter =
            [](Serializable* pThis, const bool& bNewValue) {
                reinterpret_cast<CollisionNode*>(pThis)->setIsCollisionEnabled(bNewValue);
            },
        .getter = [](Serializable* pThis) -> bool {
            return reinterpret_cast<CollisionNode*>(pThis)->isCollisionEnabled();
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
    setOnShapeChangedCallback();
}

void CollisionNode::setIsCollisionEnabled(bool bEnable) {
    if (bIsCollisionEnabled == bEnable) {
        return;
    }

    bIsCollisionEnabled = bEnable;

    if (pBody == nullptr) {
        return;
    }

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second) != nullptr) {
            // TODO: recreate compound?
            Log::warn(
                "disabling collision as part of a compound node is not implemented yet, note that "
                "when implemented this will most likely cause the whole compound to be recreated");
            return;
        }
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addRemoveBody(
        pBody, !bIsCollisionEnabled, false); // collision does not need activation to work
}

void CollisionNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    if (pNewShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("`nullptr` is not a valid shape (node \"{}\")", getNodeName()));
    }

    pShape = std::move(pNewShape);
    setOnShapeChangedCallback();

    if (!isSpawned()) {
        return;
    }

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (auto pCompound = dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second)) {
            pCompound->onChildCollisionChangedShape();
            return;
        }
    }

    if (pBody != nullptr) {
        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);
    }
}

CollisionShape& CollisionNode::getShape() const {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("collision node \"{}\" has invalid shape", getNodeName()));
    }

    return *pShape;
}

void CollisionNode::setOnShapeChangedCallback() {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    pShape->setOnChanged([this]() {
        if (!isSpawned()) {
            return;
        }

        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (auto pCompound = dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second)) {
            pCompound->onChildCollisionChangedShape();
            return;
        }

        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);
    });
}

void CollisionNode::onSpawning() {
    SpatialNode::onSpawning();

    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected collision node \"{}\" to have a valid shape when spawning", getNodeName()));
    }
    setOnShapeChangedCallback();

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second) != nullptr) {
            // Don't create collision, compound will create group collision after we spawned.
            return;
        }
    }

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().createBodyForNode(this);
}

void CollisionNode::onDespawning() {
    SpatialNode::onDespawning();

    // Clear callback.
    pShape->setOnChanged([]() {});

    if (pBody != nullptr) {
        getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
    }
}

void CollisionNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (!isSpawned()) {
        return;
    }

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (auto pCompound = dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second)) {
            // We need to adjust subshape position/rotation which means we need to recreate the compound.
            pCompound->onChildCollisionChangedShape();
            return;
        }
    }

    if (pBody == nullptr) {
        // Not created yet.
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setBodyLocationRotation(pBody, getWorldLocation(), getWorldRotation());
}

void CollisionNode::onAfterAttachedToNewParent(bool bThisNodeBeingAttached) {
    SpatialNode::onAfterAttachedToNewParent(bThisNodeBeingAttached);

    if (!isSpawned()) {
        return;
    }

    {
        const auto mtxParentNode = getParentNode();
        std::scoped_lock guard(*mtxParentNode.first);
        if (dynamic_cast<CompoundCollisionNode*>(mtxParentNode.second) != nullptr) {
            // Compound will create group collision.
            if (pBody != nullptr) {
                auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
                physicsManager.destroyBodyForNode(this);
            }
            return;
        }
    }
}
