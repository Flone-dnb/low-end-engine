#include "game/node/physics/TriggerVolumeNode.h"

// Standard.
#include <format>

// Custom.
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/geometry/shapes/CollisionShape.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "0adca195-38cf-410b-acc8-56d5e38c7c38";
}

std::string TriggerVolumeNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string TriggerVolumeNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo TriggerVolumeNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.serializables[NAMEOF_MEMBER(&TriggerVolumeNode::pShape).data()] =
        ReflectedVariableInfo<std::unique_ptr<Serializable>>{
            .setter =
                [](Serializable* pThis, std::unique_ptr<Serializable> pNewValue) {
                    auto pNewShape =
                        std::unique_ptr<CollisionShape>(dynamic_cast<CollisionShape*>(pNewValue.release()));
                    if (pNewShape == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("invalid type for variable");
                    }
                    reinterpret_cast<TriggerVolumeNode*>(pThis)->pShape = std::move(pNewShape);
                },
            .getter = [](Serializable* pThis) -> Serializable* {
                return reinterpret_cast<TriggerVolumeNode*>(pThis)->pShape.get();
            }};

    variables.bools[NAMEOF_MEMBER(&TriggerVolumeNode::bIsTriggerEnabled).data()] =
        ReflectedVariableInfo<bool>{
            .setter =
                [](Serializable* pThis, const bool& bNewValue) {
                    reinterpret_cast<TriggerVolumeNode*>(pThis)->setIsTriggerEnabled(bNewValue);
                },
            .getter = [](Serializable* pThis) -> bool {
                return reinterpret_cast<TriggerVolumeNode*>(pThis)->isTriggerEnabled();
            }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(TriggerVolumeNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<TriggerVolumeNode>(); },
        std::move(variables));
}

TriggerVolumeNode::TriggerVolumeNode() : TriggerVolumeNode("Trigger Volume Node") {}
TriggerVolumeNode::TriggerVolumeNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pShape = std::make_unique<BoxCollisionShape>();
    setOnShapeChangedCallback();
}

void TriggerVolumeNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    if (pNewShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("`nullptr` is not a valid shape (node \"{}\")", getNodeName()));
    }

    pShape = std::move(pNewShape);
    setOnShapeChangedCallback();

    if (!isSpawned()) {
        return;
    }

    if (pBody != nullptr) {
        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);
    }
}

void TriggerVolumeNode::setIsTriggerEnabled(bool bEnable) {
    if (bIsTriggerEnabled == bEnable) {
        return;
    }

    bIsTriggerEnabled = bEnable;

    if (pBody == nullptr) {
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.addRemoveBody(pBody, !bIsTriggerEnabled, true);
}

CollisionShape& TriggerVolumeNode::getShape() const {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("trigger volume node \"{}\" has invalid shape", getNodeName()));
    }

    return *pShape;
}

void TriggerVolumeNode::onSpawning() {
    SpatialNode::onSpawning();

    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "expected trigger volume node \"{}\" to have a valid shape when spawning", getNodeName()));
    }
    setOnShapeChangedCallback();

    getWorldWhileSpawned()->getGameManager().getPhysicsManager().createBodyForNode(this);
}

void TriggerVolumeNode::onDespawning() {
    SpatialNode::onDespawning();

    // Clear callback.
    pShape->setOnChanged([]() {});

    if (pBody != nullptr) {
        getWorldWhileSpawned()->getGameManager().getPhysicsManager().destroyBodyForNode(this);
    }
}

void TriggerVolumeNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (!isSpawned()) {
        return;
    }

    if (pBody == nullptr) {
        // Not created yet.
        return;
    }

    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.setBodyLocationRotation(pBody, getWorldLocation(), getWorldRotation());
}

void TriggerVolumeNode::setOnShapeChangedCallback() {
    if (pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    pShape->setOnChanged([this]() {
        if (!isSpawned()) {
            return;
        }

        auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
        physicsManager.destroyBodyForNode(this);
        physicsManager.createBodyForNode(this);
    });
}
