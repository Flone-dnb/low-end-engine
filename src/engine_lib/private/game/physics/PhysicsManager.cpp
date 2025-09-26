#include "game/physics/PhysicsManager.h"

// Standard.
#include <thread>

// Custom.
#include "misc/Error.h"
#include "misc/Profiler.hpp"
#include "game/physics/PhysicsLayers.h"
#include "game/physics/CoordinateConversions.hpp"
#include "game/node/physics/CollisionNode.h"
#include "game/node/physics/DynamicBodyNode.h"
#include "game/node/physics/KinematicBodyNode.h"
#include "game/node/physics/CompoundCollisionNode.h"
#include "game/node/physics/CharacterBodyNode.h"
#include "game/geometry/shapes/CollisionShape.h"
#include "render/PhysicsDebugDrawer.hpp"
#include "game/DebugConsole.h"

#if defined(__aarch64__) || defined(__ARM64__)
#define IS_ARM64
#endif

// External.
#ifndef IS_ARM64
#include "immintrin.h"
#if !defined(WIN32)
#include "cpuid.h"
#else
#include "isa_availability.h"
#endif
#endif
#include "Jolt/Jolt.h" // Always include Jolt.h before including any other Jolt header.
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

static void checkJoltInstructionSupport() {
#ifndef IS_ARM64

    // Check instruction set support for Jolt.
    // Even though we don't use Jolt as a dynamic library thus unsupported instructions
    // will probably just crash the program on start some instructions won't crash the program
    // but instead will just return garbage results so we make sure this won't happen.
#if defined(WIN32)
    int cpuInfo[4] = {}; // EAX, EBX, ECX, and EDX registers
#else
    uint32_t eax, ebx, ecx, edx;
#endif

    // LZCNT
    // #if defined(WIN32)
    //     __cpuid(cpuInfo, 0x80000001);
    //     const bool lzcntSupported = (cpuInfo[2] & (1 << 5)) != 0;
    // #else
    //     __cpuid_count(0x80000001, 0, eax, ebx, ecx, edx);
    //     const bool lzcntSupported = (ecx & (1 << 5)) != 0;
    // #endif

    // TZCNT
    // #if defined(WIN32)
    //     __cpuidex(cpuInfo, 7, 0);
    //     const bool tzcntSupported = (cpuInfo[1] & (1 << 3)) != 0;
    // #else
    //     __cpuid_count(7, 0, eax, ebx, ecx, edx);
    //     const bool tzcntSupported = (ebx & (1 << 3)) != 0;
    // #endif

    // F16C
    // #if defined(WIN32)
    //     __cpuid(cpuInfo, 1);
    //     const bool f16cSupported = (cpuInfo[2] & (1 << 29)) != 0;
    // #else
    //     __cpuid_count(1, 0, eax, ebx, ecx, edx);
    //     const bool f16cSupported = (ecx & (1 << 29)) != 0;
    // #endif

    // SSE4.2
#if defined(WIN32)
    const bool sse42Supported = __check_isa_support(__IA_SUPPORT_SSE42);
#else
    __cpuid(1, eax, ebx, ecx, edx);
    const bool sse42Supported = (ecx & (1 << 20)) != 0;
#endif

    // if (!sse42Supported || !lzcntSupported || !tzcntSupported || !f16cSupported) {
    if (!sse42Supported) {
        Error::showErrorAndThrowException(
            "the CPU does not support some of the required processor instructions");
    }

#endif
}

namespace {
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to create
    // more that this you'll get an error.
    constexpr unsigned int MAX_BODIES = 1024;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect
    // overlapping body pairs based on their bounding boxes and will insert them into a queue for the
    // narrowphase). If you make this buffer too small the queue will fill up and the broad phase jobs will
    // start to do narrow phase work. This is slightly less efficient.
    constexpr unsigned int MAX_BODY_PAIRS = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies)
    // are detected than this number then these contacts will be ignored and bodies will start
    // interpenetrating / fall through the world.
    constexpr unsigned int MAX_CONTACT_CONSTRAINTS = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to
    // 0 for the default settings. Should be a power of 2 in the range [1, 64], use 0 to auto detect.
    constexpr unsigned int MAX_BODY_MUTEXES = 0;
}

PhysicsManager::PhysicsManager() {
    checkJoltInstructionSupport();

    pTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(1024 * 1024); // 1 MB

    pJobSystem = std::make_unique<JPH::JobSystemThreadPool>(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        static_cast<int>(std::thread::hardware_concurrency()) - 1);

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    pPhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
    pBroadPhaseLayerInterfaceImpl = std::make_unique<BroadPhaseLayerInterfaceImpl>();
    pObjectVsBroadPhaseLayerFilterImpl = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    pObjectLayerPairFilterImpl = std::make_unique<ObjectLayerPairFilterImpl>();
    pCharVsCharCollision = std::make_unique<JPH::CharacterVsCharacterCollisionSimple>();

    pPhysicsSystem->Init(
        MAX_BODIES,
        MAX_BODY_MUTEXES,
        MAX_BODY_PAIRS,
        MAX_CONTACT_CONSTRAINTS,
        *pBroadPhaseLayerInterfaceImpl,
        *pObjectVsBroadPhaseLayerFilterImpl,
        *pObjectLayerPairFilterImpl);

    // Configure physics settings.
    auto settings = pPhysicsSystem->GetPhysicsSettings();
    settings.mAllowSleeping =
        false; // disable because sleeping bodies don't notify contact callbacks which can be inconvenient
    pPhysicsSystem->SetPhysicsSettings(settings);
    pPhysicsSystem->SetGravity(convertPosDirToJolt(glm::vec3(0.0F, 0.0F, -9.81F)));

#if defined(DEBUG)
    pPhysicsDebugDrawer = std::make_unique<PhysicsDebugDrawer>();

#if defined(ENGINE_EDITOR)
    bEnableDebugRendering = true;
#else
    DebugConsole::get().registerCommand(
        "showCollision", [this](GameInstance* pGameInstance) { bEnableDebugRendering = true; });
    DebugConsole::get().registerCommand(
        "hideCollision", [this](GameInstance* pGameInstance) { bEnableDebugRendering = false; });
#endif
#endif
}

PhysicsManager::~PhysicsManager() {
    if (!dynamicBodies.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some dynamic bodies registered");
    }
    if (!characterBodies.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some character bodies registered");
    }

    if (!pCharVsCharCollision->mCharacters.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("physics manager is being destroyed but there are still some characters registered");
    }

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsManager::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    constexpr float JOLT_UPDATE_DELTA_TIME = 1.0F / 60.0F;

    // Simulate Jolt.
    bool bUpdateHappened = false;
    timeSinceLastJoltUpdate += timeSincePrevFrameInSec;
    while (timeSinceLastJoltUpdate >= JOLT_UPDATE_DELTA_TIME) {
        timeSinceLastJoltUpdate -= JOLT_UPDATE_DELTA_TIME;

#if !defined(ENGINE_EDITOR)
        {
            PROFILE_SCOPE("onBeforePhysicsUpdate - DynamicBodyNode")
            for (const auto& pDynamicNode : dynamicBodies) {
                pDynamicNode->onBeforePhysicsUpdate(JOLT_UPDATE_DELTA_TIME);
            }
        }

        {
            PROFILE_SCOPE("onBeforePhysicsUpdate - CharacterBodyNode")
            for (const auto& pCharacterBody : characterBodies) {
                pCharacterBody->onBeforePhysicsUpdate(JOLT_UPDATE_DELTA_TIME);
                pCharacterBody->updateCharacterPosition(
                    *pPhysicsSystem, *pTempAllocator, JOLT_UPDATE_DELTA_TIME);
            }
        }
#endif

        {
            PROFILE_SCOPE("JPH::PhysicsSystem::Update")
            pPhysicsSystem->Update(JOLT_UPDATE_DELTA_TIME, 1, pTempAllocator.get(), pJobSystem.get());
        }

        bUpdateHappened = true;
    };

    // Update nodes according to simulation results.
    if (bUpdateHappened && !dynamicBodies.empty()) {
        PROFILE_SCOPE("update dynamic bodies after simulation")

        for (const auto& pDynamicBodyNode : dynamicBodies) {
            JPH::Vec3 position{};
            JPH::Quat rotation{};
            pPhysicsSystem->GetBodyInterface().GetPositionAndRotation(
                pDynamicBodyNode->pBody->GetID(), position, rotation);

            pDynamicBodyNode->setPhysicsSimulationResults(
                convertPosDirFromJolt(position), convertRotationFromJolt(rotation));
        }
    }

#if defined(DEBUG)
    if (bEnableDebugRendering) {
        PROFILE_SCOPE("draw collision")

        JPH::BodyManager::DrawSettings drawSettings;
        pPhysicsSystem->DrawBodies(drawSettings, pPhysicsDebugDrawer.get());

        pPhysicsDebugDrawer->submitDrawData();
    }
#endif
}

#if defined(DEBUG)
void PhysicsManager::setEnableDebugRendering(bool bEnable) { bEnableDebugRendering = bEnable; }
#endif

void PhysicsManager::createBodyForNode(CollisionNode* pNode) {
    PROFILE_FUNC

    if (pNode->pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have a valid shape setup", pNode->getNodeName()));
    }

    // Create shape.
    const auto shapeResult = pNode->pShape->createShape();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics shape for the node \"{}\", error: {}",
            pNode->getNodeName(),
            shapeResult.GetError().data()));
    }

    // Create body.
    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        JPH::EMotionType::Static,
        static_cast<JPH::ObjectLayer>(ObjectLayer::NON_MOVING));

    JPH::Body* pCreatedBody = pPhysicsSystem->GetBodyInterface().CreateBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    // Add to physics world.
    pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::DontActivate);

    // Save created body.
    pNode->pBody = pCreatedBody;
}

void PhysicsManager::destroyBodyForNode(CollisionNode* pNode) {
    PROFILE_FUNC

    if (pNode->pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node has no physics body",
            pNode->getNodeName()));
    }
    if (pNode->pBody->GetID().IsInvalid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node's physics body ID is "
            "invalid",
            pNode->getNodeName()));
    }

    // Remove from physics world.
    pPhysicsSystem->GetBodyInterface().RemoveBody(pNode->pBody->GetID());

    // Destroy body.
    pPhysicsSystem->GetBodyInterface().DestroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;
}

void PhysicsManager::createBodyForNode(DynamicBodyNode* pNode) {
    PROFILE_FUNC

    if (pNode->pShape == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have a valid shape setup", pNode->getNodeName()));
    }

    // Create shape.
    const auto shapeResult = pNode->pShape->createShape(pNode->density);
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics shape for the node \"{}\", error: {}",
            pNode->getNodeName(),
            shapeResult.GetError().data()));
    }

    const auto bIsKinematic = dynamic_cast<KinematicBodyNode*>(pNode) != nullptr;

    // Create body.
    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        bIsKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic,
        static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));

    bodySettings.mFriction = pNode->friction;
    if (pNode->massKg > 0.0F) {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = pNode->massKg;
    }

    JPH::Body* pCreatedBody = pPhysicsSystem->GetBodyInterface().CreateBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    // Add to physics world.
    // don't activate here if node is simulated to keep all editor-related logic in the node
    pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::DontActivate);

    // Save created body.
    pNode->pBody = pCreatedBody;

    // Register.
    if (!dynamicBodies.insert(pNode).second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was already registered as a dynamic body", pNode->getNodeName()));
    }
}

void PhysicsManager::destroyBodyForNode(DynamicBodyNode* pNode) {
    PROFILE_FUNC

    if (pNode->pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node has no physics body",
            pNode->getNodeName()));
    }
    if (pNode->pBody->GetID().IsInvalid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node's physics body ID is "
            "invalid",
            pNode->getNodeName()));
    }

    // Remove from physics world.
    pPhysicsSystem->GetBodyInterface().RemoveBody(pNode->pBody->GetID());

    // Destroy body.
    pPhysicsSystem->GetBodyInterface().DestroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;

    // Unregister.
    const auto it = dynamicBodies.find(pNode);
    if (it == dynamicBodies.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was not registered as a dynamic body", pNode->getNodeName()));
    }
    dynamicBodies.erase(it);
}

void PhysicsManager::createBodyForNode(CompoundCollisionNode* pNode) {
    PROFILE_FUNC

    const auto mtxChildNodes = pNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    if (mtxChildNodes.second.empty()) {
        Logger::get().warn(std::format(
            "expected the compound collision node \"{}\" to have child collision nodes",
            pNode->getNodeName()));
        return;
    }
    if (mtxChildNodes.second.size() == 1) {
        Logger::get().warn(std::format(
            "compound collision node \"{}\" has only 1 child collision node, in this case it's better to "
            "create a single collision node instead of using a compound",
            pNode->getNodeName()));
    }

    JPH::Array<JPH::Ref<JPH::Shape>> vShapes;
    vShapes.reserve(mtxChildNodes.second.size());
    JPH::StaticCompoundShapeSettings compoundSettings;
    for (const auto& pChildNode : mtxChildNodes.second) {
#if defined(DEBUG)
        if (!pChildNode->getChildNodes().second.empty()) [[unlikely]] {
            Logger::get().warn(std::format(
                "child node \"{}\" of a compound node \"{}\" has child nodes, these child nodes will not be "
                "grouped into a compound, only direct child nodes of a compound node will be grouped into a "
                "compound",
                pChildNode->getNodeName(),
                pNode->getNodeName()));
        }
#endif

        // Cast type.
        const auto pCollisionNode = dynamic_cast<CollisionNode*>(pChildNode);
        if (pCollisionNode == nullptr) [[unlikely]] {
            Logger::get().error(std::format(
                "expected the child node \"{}\" of a compound node \"{}\" to be a collision node",
                pChildNode->getNodeName(),
                pNode->getNodeName()));
            continue;
        }

        // Create shape.
        const auto shapeResult = pCollisionNode->pShape->createShape();
        if (!shapeResult.IsValid()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "failed to create a physics shape for the node \"{}\" which is child node of a compound node "
                "\"{}\", error: {}",
                pCollisionNode->getNodeName(),
                pNode->getNodeName(),
                shapeResult.GetError().data()));
        }
        const auto& pShape = shapeResult.Get();
        vShapes.push_back(pShape);

        compoundSettings.AddShape(
            convertPosDirToJolt(pCollisionNode->getRelativeLocation()),
            convertRotationToJolt(pCollisionNode->getRelativeRotation()),
            pShape);
    }

    if (vShapes.empty()) {
        return;
    }

    // Create compound shape.
    const auto shapeResult = compoundSettings.Create();
    if (!shapeResult.IsValid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics shape for the compound collision node \"{}\", error: {}",
            pNode->getNodeName(),
            shapeResult.GetError().data()));
    }

    // Create body.
    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        JPH::EMotionType::Static,
        static_cast<JPH::ObjectLayer>(ObjectLayer::NON_MOVING));

    JPH::Body* pCreatedBody = pPhysicsSystem->GetBodyInterface().CreateBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    // Add to physics world.
    pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::DontActivate);

    // Save created body.
    pNode->pBody = pCreatedBody;
}

void PhysicsManager::destroyBodyForNode(CompoundCollisionNode* pNode) {
    PROFILE_FUNC

    if (pNode->pBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node has no physics body",
            pNode->getNodeName()));
    }
    if (pNode->pBody->GetID().IsInvalid()) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "the node \"{}\" requested its physics body to be destroyed but this node's physics body ID is "
            "invalid",
            pNode->getNodeName()));
    }

    // Remove from physics world.
    pPhysicsSystem->GetBodyInterface().RemoveBody(pNode->pBody->GetID());

    // Destroy body.
    pPhysicsSystem->GetBodyInterface().DestroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;
}

void PhysicsManager::createBodyForNode(CharacterBodyNode* pNode) {
    PROFILE_FUNC

    // Prepare settings.
    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = glm::radians(pNode->getMaxWalkSlopeAngle());
    settings->mShape = pNode->createCharacterShape();
    settings->mSupportingVolume = JPH::Plane(
        convertPosDirToJolt(glm::vec3(Globals::WorldDirection::up)), -pNode->pCollisionShape->getRadius());
    settings->mInnerBodyLayer = static_cast<JPH::ObjectLayer>(ObjectLayer::NON_MOVING);

    // Create character.
    pNode->pCharacterBody = new JPH::CharacterVirtual(
        settings,
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        0,
        pPhysicsSystem.get());

    pNode->pCharacterBody->SetCharacterVsCharacterCollision(pCharVsCharCollision.get());

    // Register.
    if (!characterBodies.insert(pNode).second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was already registered as a character body", pNode->getNodeName()));
    }
}

void PhysicsManager::destroyBodyForNode(CharacterBodyNode* pNode) {
    PROFILE_FUNC

    pNode->pCharacterBody = nullptr;

    // Unregister.
    const auto it = characterBodies.find(pNode);
    if (it == characterBodies.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was not registered as a character body", pNode->getNodeName()));
    }
    characterBodies.erase(it);
}

void PhysicsManager::setBodyLocationRotation(
    JPH::Body* pBody, const glm::vec3& location, const glm::vec3& rotation) {
    PROFILE_FUNC

    pPhysicsSystem->GetBodyInterface().SetPositionAndRotation(
        pBody->GetID(),
        convertPosDirToJolt(location),
        convertRotationToJolt(rotation),
        JPH::EActivation::DontActivate);
}

void PhysicsManager::setBodyActiveState(JPH::Body* pBody, bool bActivate) {
    PROFILE_FUNC

    if (bActivate) {
        pPhysicsSystem->GetBodyInterface().ActivateBody(pBody->GetID());
    } else {
        pPhysicsSystem->GetBodyInterface().DeactivateBody(pBody->GetID());
    }
}

void PhysicsManager::addImpulseToBody(JPH::Body* pBody, const glm::vec3& impulse) {
    pPhysicsSystem->GetBodyInterface().AddImpulse(pBody->GetID(), convertPosDirToJolt(impulse));
}

void PhysicsManager::addAngularImpulseToBody(JPH::Body* pBody, const glm::vec3& impulse) {
    pPhysicsSystem->GetBodyInterface().AddAngularImpulse(pBody->GetID(), convertPosDirToJolt(impulse));
}

void PhysicsManager::addForce(JPH::Body* pBody, const glm::vec3& force) {
    pPhysicsSystem->GetBodyInterface().AddForce(pBody->GetID(), convertPosDirToJolt(force));
}

void PhysicsManager::setLinearVelocity(JPH::Body* pBody, const glm::vec3& velocity) {
    pPhysicsSystem->GetBodyInterface().SetLinearVelocity(pBody->GetID(), convertPosDirToJolt(velocity));
}

void PhysicsManager::setAngularVelocity(JPH::Body* pBody, const glm::vec3& velocity) {
    pPhysicsSystem->GetBodyInterface().SetAngularVelocity(pBody->GetID(), convertPosDirToJolt(velocity));
}

glm::vec3 PhysicsManager::getLinearVelocity(JPH::Body* pBody) {
    return convertPosDirFromJolt(pPhysicsSystem->GetBodyInterface().GetLinearVelocity(pBody->GetID()));
}

glm::vec3 PhysicsManager::getAngularVelocity(JPH::Body* pBody) {
    return convertPosDirFromJolt(pPhysicsSystem->GetBodyInterface().GetAngularVelocity(pBody->GetID()));
}

void PhysicsManager::optimizeBroadPhase() {
    PROFILE_FUNC

    pPhysicsSystem->OptimizeBroadPhase();
}

glm::vec3 PhysicsManager::getGravity() { return convertPosDirFromJolt(pPhysicsSystem->GetGravity()); }

JPH::PhysicsSystem& PhysicsManager::getPhysicsSystem() { return *pPhysicsSystem; }