#include "game/node/physics/CharacterBodyNode.h"

// Standard.
#include <format>

// Custom.
#include "game/geometry/shapes/CollisionShape.h"
#include "game/World.h"
#include "game/GameManager.h"
#include "game/physics/PhysicsManager.h"
#include "game/physics/CoordinateConversions.hpp"
#include "game/physics/PhysicsLayers.h"

// External.
#include "nameof.hpp"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/PhysicsSystem.h"

namespace {
    constexpr std::string_view sTypeGuid = "c2fa0ee4-c469-4bc0-b610-efe6c5b85e7a";
}

std::string CharacterBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string CharacterBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo CharacterBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.serializables[NAMEOF_MEMBER(&CharacterBodyNode::pCollisionShape).data()] =
        ReflectedVariableInfo<std::unique_ptr<Serializable>>{
            .setter =
                [](Serializable* pThis, std::unique_ptr<Serializable> pNewValue) {
                    auto pNewShape = std::unique_ptr<CapsuleCollisionShape>(
                        dynamic_cast<CapsuleCollisionShape*>(pNewValue.release()));
                    if (pNewShape == nullptr) [[unlikely]] {
                        Error::showErrorAndThrowException("invalid type for variable");
                    }
                    reinterpret_cast<CharacterBodyNode*>(pThis)->pCollisionShape = std::move(pNewShape);
                },
            .getter = [](Serializable* pThis) -> Serializable* {
                return reinterpret_cast<CharacterBodyNode*>(pThis)->pCollisionShape.get();
            }};

    variables.floats[NAMEOF_MEMBER(&CharacterBodyNode::maxWalkSlopeAngleDeg).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<CharacterBodyNode*>(pThis)->setMaxWalkSlopeAngle(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<CharacterBodyNode*>(pThis)->getMaxWalkSlopeAngle();
            }};

    variables.floats[NAMEOF_MEMBER(&CharacterBodyNode::maxStepHeight).data()] = ReflectedVariableInfo<float>{
        .setter =
            [](Serializable* pThis, const float& newValue) {
                reinterpret_cast<CharacterBodyNode*>(pThis)->setMaxStepHeight(newValue);
            },
        .getter = [](Serializable* pThis) -> float {
            return reinterpret_cast<CharacterBodyNode*>(pThis)->getMaxStepHeight();
        }};

    return TypeReflectionInfo(
        SpatialNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(CharacterBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<CharacterBodyNode>(); },
        std::move(variables));
}

CharacterBodyNode::CharacterBodyNode() : CharacterBodyNode("Character Body Node") {}
CharacterBodyNode::CharacterBodyNode(const std::string& sNodeName) : SpatialNode(sNodeName) {
    pCollisionShape = std::make_unique<CapsuleCollisionShape>();
    pCollisionShape->setOnChanged([this]() { recreateBodyIfSpawned(); });
}

CharacterBodyNode::~CharacterBodyNode() {}

void CharacterBodyNode::setMaxWalkSlopeAngle(float degrees) {
    maxWalkSlopeAngleDeg = degrees;
    recreateBodyIfSpawned();
}

void CharacterBodyNode::setMaxStepHeight(float newMaxStepHeight) {
    maxStepHeight = newMaxStepHeight;
    recreateBodyIfSpawned();
}

CharacterBodyNode::GroundState CharacterBodyNode::getGroundState() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

    return static_cast<GroundState>(pCharacterBody->GetGroundState());
}

void CharacterBodyNode::recreateBodyIfSpawned() {
    if (!isSpawned()) {
        return;
    }

    destroyCharacterBody();
    createCharacterBody();
}

JPH::Ref<JPH::Shape> CharacterBodyNode::createCharacterShape() {
    if (pCollisionShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected the shape to be valid");
    }

    auto shapeResult = pCollisionShape->createShape();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics shape for the node \"{}\", error: {}",
            getNodeName(),
            shapeResult.GetError().data()));
    }

    // Adjust to have bottom on (0, 0, 0).
    shapeResult = JPH::RotatedTranslatedShapeSettings(
                      convertPosDirToJolt(glm::vec3(0.0F, 0.0F, pCollisionShape->getHalfHeight())),
                      JPH::Quat::sIdentity(),
                      shapeResult.Get())
                      .Create();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics shape for the node \"{}\", error: {}",
            getNodeName(),
            shapeResult.GetError().data()));
    }

    return shapeResult.Get();
}

void CharacterBodyNode::createCharacterBody() {
    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }
    if (pCharacterBody != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be empty on node \"{}\"", getNodeName()));
    }

    // Create body.
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.createBodyForNode(this);
}

void CharacterBodyNode::destroyCharacterBody() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    // Destroy body.
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    physicsManager.destroyBodyForNode(this);
}

void CharacterBodyNode::onSpawning() {
    SpatialNode::onSpawning();

    createCharacterBody();
}

void CharacterBodyNode::onDespawning() {
    SpatialNode::onDespawning();

    destroyCharacterBody();
}

void CharacterBodyNode::onBeforePhysicsUpdate(float deltaTime) { pCharacterBody->UpdateGroundVelocity(); }

void CharacterBodyNode::updateCharacterPosition(
    JPH::PhysicsSystem& physicsSystem, JPH::TempAllocator& tempAllocator, float deltaTime) {
    PROFILE_FUNC

    // Prepare to update the position.
    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mStickToFloorStepDown = -pCharacterBody->GetUp() * static_cast<float>(glm::vec3(0.0F, 0.0F, -0.25F).length());
    updateSettings.mWalkStairsStepUp =
        pCharacterBody->GetUp() * static_cast<float>(glm::vec3(0.0F, 0.0F, maxStepHeight).length());

    // Update position.
    pCharacterBody->ExtendedUpdate(
        deltaTime,
        -pCharacterBody->GetUp() * physicsSystem.GetGravity().Length(),
        updateSettings,
        physicsSystem.GetDefaultBroadPhaseLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING)),
        physicsSystem.GetDefaultLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING)),
        {},
        {},
        tempAllocator);

    bIsApplyingUpdateResults = true;
    {
        setWorldLocation(convertPosDirFromJolt(pCharacterBody->GetPosition()));
    }
    bIsApplyingUpdateResults = false;
}

void CharacterBodyNode::onWorldLocationRotationScaleChanged() {
    SpatialNode::onWorldLocationRotationScaleChanged();

    if (pCharacterBody == nullptr) {
        return;
    }

    if (!bIsApplyingUpdateResults) {
        pCharacterBody->SetPosition(convertPosDirToJolt(getWorldLocation()));
        pCharacterBody->SetUp(convertPosDirToJolt(getWorldUpDirection()));
        pCharacterBody->SetRotation(convertRotationToJolt(getWorldRotation()));
    }

#if defined(DEBUG)
    if (bIsApplyingUpdateResults && !bWarnedAboutFallingOutOfWorld) {
        const auto worldLocation = getWorldLocation();
        if (worldLocation.z < -1000.0F) {
            Logger::get().warn(std::format(
                "character body node \"{}\" seems to be falling out of the world, its world location is "
                "({}, {}, {})",
                getNodeName(),
                worldLocation.x,
                worldLocation.y,
                worldLocation.z));
            bWarnedAboutFallingOutOfWorld = true;
        }
    }
#endif
}

void CharacterBodyNode::setLinearVelocity(const glm::vec3& velocity) {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    pCharacterBody->SetLinearVelocity(convertPosDirToJolt(velocity));
}

glm::vec3 CharacterBodyNode::getLinearVelocity() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    return convertPosDirFromJolt(pCharacterBody->GetLinearVelocity());
}

glm::vec3 CharacterBodyNode::getGroundNormal() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    return convertPosDirFromJolt(pCharacterBody->GetGroundNormal());
}

glm::vec3 CharacterBodyNode::getGroundVelocity() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    return convertPosDirFromJolt(pCharacterBody->GetGroundVelocity());
}

bool CharacterBodyNode::isSlopeTooSteep(const glm::vec3& normal) {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the body to be valid on node \"{}\"", getNodeName()));
    }

    return pCharacterBody->IsSlopeTooSteep(convertPosDirToJolt(normal));
}

glm::vec3 CharacterBodyNode::getGravity() {
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    return physicsManager.getGravity();
}