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

void DynamicBodyNode::setOnShapeChangedCallback() {
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

void DynamicBodyNode::setShape(std::unique_ptr<CollisionShape> pNewShape) {
    pShape = std::move(pNewShape);
    setOnShapeChangedCallback();

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
    }
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

void DynamicBodyNode::onSpawning() {
    SpatialNode::onSpawning();

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

void DynamicBodyNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

#if !defined(ENGINE_EDITOR)
    if (bIsSimulated) {
#if defined(DEBUG)
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
#endif

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
