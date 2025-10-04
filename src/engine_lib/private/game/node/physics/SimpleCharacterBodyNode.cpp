#include "game/node/physics/SimpleCharacterBodyNode.h"

// Custom.
#include "math/MathHelpers.hpp"
#include "game/geometry/shapes/CollisionShape.h"

// External.
#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "825a909d-be1b-43b9-89d6-806dcb800191";
}

std::string SimpleCharacterBodyNode::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string SimpleCharacterBodyNode::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo SimpleCharacterBodyNode::getReflectionInfo() {
    ReflectedVariables variables;

    variables.floats[NAMEOF_MEMBER(&SimpleCharacterBodyNode::movementSpeed).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->setMovementSpeed(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getMovementSpeed();
            }};

    variables.floats[NAMEOF_MEMBER(&SimpleCharacterBodyNode::jumpPower).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->setJumpPower(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getJumpPower();
            }};

    variables.floats[NAMEOF_MEMBER(&SimpleCharacterBodyNode::gravityMultiplier).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->setGravityMultiplier(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getGravityMultiplier();
            }};

    variables.floats[NAMEOF_MEMBER(&SimpleCharacterBodyNode::airMovementControlFactor).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->setAirMovementControlFactor(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getAirMovementControlFactor();
            }};

    variables.floats[NAMEOF_MEMBER(&SimpleCharacterBodyNode::crouchingHeightFactor).data()] =
        ReflectedVariableInfo<float>{
            .setter =
                [](Serializable* pThis, const float& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->setCrouchingHeightFactor(newValue);
                },
            .getter = [](Serializable* pThis) -> float {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getCrouchingHeightFactor();
            }};

    variables.bools[NAMEOF_MEMBER(&SimpleCharacterBodyNode::bIsCrouching).data()] =
        ReflectedVariableInfo<bool>{
            .setter =
                [](Serializable* pThis, const bool& newValue) {
                    reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->trySetIsCrouching(newValue);
                },
            .getter = [](Serializable* pThis) -> bool {
                return reinterpret_cast<SimpleCharacterBodyNode*>(pThis)->getIsCrouching();
            }};

    return TypeReflectionInfo(
        CharacterBodyNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SimpleCharacterBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SimpleCharacterBodyNode>(); },
        std::move(variables));
}

SimpleCharacterBodyNode::SimpleCharacterBodyNode() : SimpleCharacterBodyNode("Simple Character Body Node") {}
SimpleCharacterBodyNode::SimpleCharacterBodyNode(const std::string& sNodeName)
    : CharacterBodyNode(sNodeName) {
}

SimpleCharacterBodyNode::~SimpleCharacterBodyNode() {}

void SimpleCharacterBodyNode::setForwardMovementInput(float input) { movementInput.x = input; }
void SimpleCharacterBodyNode::setRightMovementInput(float input) { movementInput.y = input; }

void SimpleCharacterBodyNode::jump(bool bEvenIfInAir) {
    bWantsToJump = true;
    bWantsToJumpEvenIfInAir = bEvenIfInAir;
}

void SimpleCharacterBodyNode::setMovementSpeed(float newSpeed) { movementSpeed = newSpeed; }

void SimpleCharacterBodyNode::setJumpPower(float newJumpPower) { jumpPower = newJumpPower; }

void SimpleCharacterBodyNode::setGravityMultiplier(float newMultiplier) { gravityMultiplier = newMultiplier; }

void SimpleCharacterBodyNode::setAirMovementControlFactor(float factor) {
    airMovementControlFactor = std::clamp(factor, 0.0F, 1.0F);
}

void SimpleCharacterBodyNode::setCrouchingHeightFactor(float factor) {
    crouchingHeightFactor = std::clamp(factor, 0.1F, 1.0F);
}

bool SimpleCharacterBodyNode::trySetIsCrouching(bool bIsCrouching) {
    if (this->bIsCrouching == bIsCrouching) {
        return true;
    }

    CapsuleCollisionShape newShape = getBodyShape();

    bool bSuccess = false;
    if (!this->bIsCrouching) {
        // Standing -> crouching.
        charHalfHeightBeforeCrouching = getBodyShape().getHalfHeight();
        newShape.setHalfHeight(charHalfHeightBeforeCrouching * crouchingHeightFactor);
        bSuccess = trySetNewShape(newShape);
        this->bIsCrouching = bSuccess;
    } else {
        // Crouchcing -> standing.
        newShape.setHalfHeight(charHalfHeightBeforeCrouching);
        bSuccess = trySetNewShape(newShape);
        this->bIsCrouching = !bSuccess;
    }

    return bSuccess;
}

void SimpleCharacterBodyNode::onBeforePhysicsUpdate(float deltaTime) {
    PROFILE_FUNC

    CharacterBodyNode::onBeforePhysicsUpdate(deltaTime);

    MathHelpers::fixDiagonalMovementSpeedup(movementInput);

    const auto upDirection = getWorldUpDirection();
    const auto groundState = getGroundState();

    const auto verticalVelocity = glm::dot(getLinearVelocity(), upDirection) * upDirection;
    const auto groundVelocity = getGroundVelocity();

    // Setup base velocity.
    glm::vec3 newVelocity = glm::vec3(0.0F);
    if (groundState == GroundState::OnGround && !isSlopeTooSteep(getGroundNormal())) {
        newVelocity = groundVelocity;
        if (bWantsToJump) {
            newVelocity += upDirection * jumpPower;
        }
    } else {
        if (bWantsToJumpEvenIfInAir) {
            newVelocity += upDirection * jumpPower;
        } else {
            newVelocity = verticalVelocity;
        }
    }

    // Apply gravity.
    newVelocity += getGravity() * gravityMultiplier * deltaTime;

    // Apply movement.
    const auto movementVec =
        getWorldForwardDirection() * movementInput.x + getWorldRightDirection() * movementInput.y;
    if (groundState == GroundState::OnGround || groundState == GroundState::OnSteepGround) {
        newVelocity += movementVec * movementSpeed;
    } else {
        const auto horizontalVelocity = getLinearVelocity() - verticalVelocity;
        newVelocity += horizontalVelocity * (1.0F - airMovementControlFactor) +
                       movementVec * movementSpeed * airMovementControlFactor;
    }

    // Set new velocity.
    setLinearVelocity(newVelocity);

    bWantsToJump = false;
    bWantsToJumpEvenIfInAir = false;
}