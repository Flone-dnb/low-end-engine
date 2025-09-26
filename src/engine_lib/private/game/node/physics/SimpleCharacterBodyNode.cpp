#include "game/node/physics/SimpleCharacterBodyNode.h"

// Custom.
#include "math/MathHelpers.hpp"

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

    return TypeReflectionInfo(
        CharacterBodyNode::getTypeGuidStatic(),
        NAMEOF_SHORT_TYPE(SimpleCharacterBodyNode).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<SimpleCharacterBodyNode>(); },
        std::move(variables));
}

SimpleCharacterBodyNode::SimpleCharacterBodyNode() : SimpleCharacterBodyNode("Simple Character Body Node") {}
SimpleCharacterBodyNode::SimpleCharacterBodyNode(const std::string& sNodeName)
    : CharacterBodyNode(sNodeName) {}

SimpleCharacterBodyNode::~SimpleCharacterBodyNode() {}

void SimpleCharacterBodyNode::setMovementInput(const glm::vec2& input) { movementInput = input; }

void SimpleCharacterBodyNode::jump() { bWantsToJump = true; }

void SimpleCharacterBodyNode::setMovementSpeed(float newSpeed) { movementSpeed = newSpeed; }

void SimpleCharacterBodyNode::setJumpPower(float newJumpPower) { jumpPower = newJumpPower; }

void SimpleCharacterBodyNode::onBeforePhysicsUpdate(float deltaTime) {
    PROFILE_FUNC

    CharacterBodyNode::onBeforePhysicsUpdate(deltaTime);

    // Fix diagonal movement.
    movementInput = MathHelpers::normalizeSafely(movementInput);

    const auto upDirection = getWorldUpDirection();
    const auto groundState = getGroundState();

    const auto verticalVelocity = glm::dot(getLinearVelocity(), upDirection) * upDirection;
    const auto groundVelocity = getGroundVelocity();

    // Setup base velocity.
    glm::vec3 newVelocity = glm::vec3(0.0F);
    if (groundState == GroundState::OnGround && !isSlopeTooSteep(getGroundNormal())) {
        newVelocity = groundVelocity;
        if (bWantsToJump){
            newVelocity += upDirection * jumpPower;
        }
    }else{
        newVelocity = verticalVelocity;
    }

    // Apply gravity.
    newVelocity += getGravity() * deltaTime;

    // Apply movement.
    if (groundState == GroundState::OnGround || groundState == GroundState::OnSteepGround) {
        newVelocity += glm::vec3(movementInput.x, 0.0F, movementInput.y) * movementSpeed;
    } else {
        const auto horizontalVelocity = getLinearVelocity() - verticalVelocity;
        newVelocity += horizontalVelocity;
    }

    // Set new velocity.
    setLinearVelocity(newVelocity);

    bWantsToJump = false;
}