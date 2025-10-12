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

    pContactListener = std::make_unique<ContactListener>(this);
}

CharacterBodyNode::~CharacterBodyNode() {}

std::optional<CharacterBodyNode::RayCastHit> CharacterBodyNode::castRayUntilHit(
    const glm::vec3& rayStartPosition,
    const glm::vec3& rayEndPosition,
    bool bIgnoreThisCharacter,
    bool bIgnoreTriggers) {
    PROFILE_FUNC

    if (!isSpawned()) [[unlikely]] {
        Error::showErrorAndThrowException("ray cast function can only be used on spawned nodes");
    }

    std::vector<JPH::BodyID> vIgnoredBodies;
    if (bIgnoreThisCharacter && !pCharacterBody->GetInnerBodyID().IsInvalid()) {
        vIgnoredBodies.push_back(pCharacterBody->GetInnerBodyID());
    }

    const auto pWorld = getWorldWhileSpawned();
    auto& physicsManager = pWorld->getGameManager().getPhysicsManager();

    std::optional<PhysicsManager::RayCastHit> hitResult;
    do {
        // Cast ray.
        hitResult = pWorld->getGameManager().getPhysicsManager().castRayUntilHit(
            rayStartPosition, rayEndPosition, vIgnoredBodies);
        if (!hitResult.has_value()) {
            return {};
        }

        if (bIgnoreTriggers && physicsManager.isBodySensor(hitResult->bodyId)) {
            vIgnoredBodies.push_back(hitResult->bodyId);
            continue;
        } else {
            break;
        }
    } while (true); // TODO: rework I guess

    // Get hit node.
    const auto pHitNode = pWorld->getSpawnedNodeById(physicsManager.getUserDataFromBody(hitResult->bodyId));
    if (pHitNode == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" is unable to determine hit node from ray cast result", getNodeName()));
    }

    return CharacterBodyNode::RayCastHit{
        .pHitNode = pHitNode, .hitPosition = hitResult->hitPosition, .hitNormal = hitResult->hitNormal};
}

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

Node* CharacterBodyNode::getGroundNodeIfExists() {
    if (pCharacterBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to be spawned", getNodeName()));
    }

    const auto pWorld = getWorldWhileSpawned();
    auto& physicsManager = pWorld->getGameManager().getPhysicsManager();

    const auto bodyId = pCharacterBody->GetGroundBodyID();
    if (bodyId.IsInvalid()) {
        return nullptr;
    }

    return pWorld->getSpawnedNodeById(physicsManager.getUserDataFromBody(bodyId));
}

void CharacterBodyNode::recreateBodyIfSpawned() {
    if (!isSpawned()) {
        return;
    }

    destroyCharacterBody();
    createCharacterBody();
}

JPH::Ref<JPH::Shape>
CharacterBodyNode::createAdjustedJoltShapeForCharacter(const CapsuleCollisionShape& shape) {
    // Create shape.
    auto shapeResult = shape.createShape();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to create a physics shape, error: {}", shapeResult.GetError().data()));
    }

    // Adjust to have bottom on (0, 0, 0).
    shapeResult = JPH::RotatedTranslatedShapeSettings(
                      convertPosDirToJolt(glm::vec3(0.0F, shape.getHalfHeight() + shape.getRadius(), 0.0F)),
                      JPH::Quat::sIdentity(),
                      shapeResult.Get())
                      .Create();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("failed to create a physics shape, error: {}", shapeResult.GetError().data()));
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

    // Character's up should always be world up.
    pCharacterBody->SetUp(convertPosDirToJolt(Globals::WorldDirection::up));
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

void CharacterBodyNode::onBeforePhysicsUpdate(float deltaTime) {
    pCharacterBody->UpdateGroundVelocity();

#if defined(DEBUG)
    bIsInPhysicsTick = true; // physics engine then calls update in this node
#endif
}

bool CharacterBodyNode::trySetNewShape(CapsuleCollisionShape& newShape) {
    if (pCharacterBody == nullptr) {
        pCollisionShape->setHalfHeight(newShape.getHalfHeight());
        pCollisionShape->setRadius(newShape.getRadius());
        return true;
    }

#if defined(DEBUG)
    if (bIsInPhysicsTick) [[unlikely]] {
        // We probably shouldn't change shapes during the physics update.
        Error::showErrorAndThrowException(std::format(
            "this function cannot be called while in physics update (node \"{}\")", getNodeName()));
    }
#endif

    const JPH::Ref<JPH::Shape> pNewShape = createAdjustedJoltShapeForCharacter(newShape);
    auto& physicsManager = getWorldWhileSpawned()->getGameManager().getPhysicsManager();
    auto& physicsSystem = physicsManager.getPhysicsSystem();

    // Prepare body filter.
    std::unique_ptr<JPH::BodyFilter> pBodyFilter = std::make_unique<JPH::BodyFilter>();
    if (pCharacterBody->IsSupported()) {
        pBodyFilter = std::make_unique<JPH::IgnoreSingleBodyFilter>(pCharacterBody->GetGroundBodyID());
    }

    const auto bSuccess = pCharacterBody->SetShape(
        pNewShape,
        1.5F * physicsSystem.GetPhysicsSettings().mPenetrationSlop,
        physicsSystem.GetDefaultBroadPhaseLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING)),
        physicsSystem.GetDefaultLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING)),
        *pBodyFilter,
        {},
        physicsManager.getTempAllocator());

    if (bSuccess) {
        pCharacterBody->SetInnerBodyShape(pNewShape.GetPtr());

        pCollisionShape->setHalfHeight(newShape.getHalfHeight());
        pCollisionShape->setRadius(newShape.getRadius());
    }

    return bSuccess;
}

void CharacterBodyNode::updateCharacterPosition(
    JPH::PhysicsSystem& physicsSystem, JPH::TempAllocator& tempAllocator, float deltaTime) {
    PROFILE_FUNC

    // Prepare to update the position.
    JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
    updateSettings.mStickToFloorStepDown = -pCharacterBody->GetUp() * 0.2F;
    updateSettings.mWalkStairsStepUp = pCharacterBody->GetUp() * maxStepHeight;

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
        pCharacterBody->SetRotation(convertRotationToJolt(getWorldRotation()));
    }

#if defined(DEBUG)
    if (bIsApplyingUpdateResults && !bWarnedAboutFallingOutOfWorld) {
        const auto worldLocation = getWorldLocation();
        if (worldLocation.y < -1000.0F) {
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

#if defined(DEBUG)
    if (!bIsInPhysicsTick) [[unlikely]] {
        Error::showErrorAndThrowException(
            "this and similar physics functions should be called in onBeforePhysicsUpdate");
    }
#endif

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

void CharacterBodyNode::ContactListener::OnContactAdded(
    const JPH::CharacterVirtual* character,
    const JPH::BodyID& hitBodyId,
    const JPH::SubShapeID& hitSubShapeId,
    JPH::RVec3Arg contactPosition,
    JPH::Vec3Arg contactNormal,
    JPH::CharacterContactSettings& ioSettings) {
    // AFAIK this is called from Jolt's thread pool while the body interface is locked so giving control to
    // the user code now might be a bad idea (in case the user will try to use our physics functions which
    // will try to lock the body interface). So we will just put this event in a queue and process after the
    // physics update.
    std::scoped_lock guard(pOwner->mtxContactsToProcess.first);

    pOwner->mtxContactsToProcess.second.push(CharacterBodyNode::BodyContactInfo{
        .bIsAdded = true,
        .hitBodyId = hitBodyId,
        .hitWorldPosition = convertPosDirFromJolt(contactPosition),
        .hitNormal = convertPosDirFromJolt(contactNormal)});
}

void CharacterBodyNode::ContactListener::OnContactRemoved(
    const JPH::CharacterVirtual* character,
    const JPH::BodyID& hitBodyId,
    const JPH::SubShapeID& hitSubShapeId) {
    std::scoped_lock guard(pOwner->mtxContactsToProcess.first);

    pOwner->mtxContactsToProcess.second.push(
        CharacterBodyNode::BodyContactInfo{.bIsAdded = false, .hitBodyId = hitBodyId});
}

void CharacterBodyNode::processContactEvents() {
    std::scoped_lock guard(mtxContactsToProcess.first);

    const auto pWorld = getWorldWhileSpawned();
    auto& physicsManager = pWorld->getGameManager().getPhysicsManager();

    while (!mtxContactsToProcess.second.empty()) {
        auto& info = mtxContactsToProcess.second.front();

        // Get node.
        const auto pNode = pWorld->getSpawnedNodeById(physicsManager.getUserDataFromBody(info.hitBodyId));
        if (pNode == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format("unable to determine hit node from body id on node \"{}\"", getNodeName()));
        }

        // Notify user.
        if (info.bIsAdded) {
            onContactAdded(pNode, info.hitWorldPosition, info.hitNormal);
        } else {
            onContactRemoved(pNode);
        }

        mtxContactsToProcess.second.pop();
    }
}
