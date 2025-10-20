#include "game/physics/PhysicsManager.h"
#include "game/physics/PhysicsManager.h"

// Standard.
#include <thread>

// Custom.
#include "misc/Error.h"
#include "misc/Profiler.hpp"
#include "game/GameManager.h"
#include "game/physics/PhysicsLayers.h"
#include "game/physics/CoordinateConversions.hpp"
#include "game/node/physics/CollisionNode.h"
#include "game/node/physics/SimulatedBodyNode.h"
#include "game/node/physics/MovingBodyNode.h"
#include "game/node/physics/CompoundCollisionNode.h"
#include "game/node/physics/CharacterBodyNode.h"
#include "game/node/physics/TriggerVolumeNode.h"
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
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"

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

/** A listener class that receives collision contact events. */
class ContactListener : public JPH::ContactListener {
public:
    /**
     * Constructor.
     *
     * @param pManager Physics manager.
     */
    ContactListener(PhysicsManager* pManager) : pManager(pManager) {}
    virtual ~ContactListener() override = default;

    /**
     * @param inBody1 Body 1.
     * @param inBody2 Body 2.
     * @param inManifold Info.
     * @param ioSettings settings.
     */
    virtual void OnContactAdded(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override {
        // Note: this function is called from Jolt's thread pool when all bodies are locked.

        // For now we only care about sensor contacts.
        const JPH::Body* pSensorBody = nullptr;
        const JPH::Body* pOtherBody = nullptr;
        if (inBody1.IsSensor()) {
            pSensorBody = &inBody1;
            pOtherBody = &inBody2;
        } else if (inBody2.IsSensor()) {
            pSensorBody = &inBody2;
            pOtherBody = &inBody1;
        } else {
            return;
        }

        auto& mtxData = pManager->mtxContactData;
        std::scoped_lock guard(mtxData.first);

        mtxData.second.newContactsAdded.push(PhysicsManager::ContactInfo{
            .bIsAdded = true,
            .iSensorNodeId = pSensorBody->GetUserData(),
            .iOtherNodeId = pOtherBody->GetUserData(),
            .worldNormal = convertPosDirFromJolt(inManifold.mWorldSpaceNormal),
            .contactPointLocation = convertPosDirFromJolt(inManifold.GetWorldSpaceContactPointOn1(0)),
        });

        mtxData.second.activeSensorContacts[JPH::SubShapeIDPair(
            inBody1.GetID(), inManifold.mSubShapeID1, inBody2.GetID(), inManifold.mSubShapeID2)] =
            PhysicsManager::SensorContactInfo{
                .iSensorNodeId = pSensorBody->GetUserData(), .iOtherNodeId = pOtherBody->GetUserData()};
    }

    /**
     * @param inSubShapePair Contact info.
     */
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        // Note: this function is called from Jolt's thread pool when all bodies are locked.

        // Body can be destroyed at this point so we can't use it.
        auto& mtxData = pManager->mtxContactData;
        std::scoped_lock guard(mtxData.first);

        const auto it = mtxData.second.activeSensorContacts.find(inSubShapePair);
        if (it == mtxData.second.activeSensorContacts.end()) {
            return;
        }

        mtxData.second.newContactsRemoved.push(PhysicsManager::ContactInfo{
            .bIsAdded = false,
            .iSensorNodeId = it->second.iSensorNodeId,
            .iOtherNodeId = it->second.iOtherNodeId});

        mtxData.second.activeSensorContacts.erase(it);
    }

private:
    /** Physics manager. */
    PhysicsManager* const pManager = nullptr;
};

PhysicsManager::PhysicsManager(GameManager* pGameManager) : pGameManager(pGameManager) {
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
    pContactListener = std::make_unique<ContactListener>(this);

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
    pPhysicsSystem->SetContactListener(pContactListener.get());

#if defined(DEBUG)
    pPhysicsDebugDrawer = std::make_unique<PhysicsDebugDrawer>();

#if defined(ENGINE_EDITOR)
    bEnableDebugRendering = true;
#else
    DebugConsole::registerCommand(
        "showCollision", [this](GameInstance* pGameInstance) { bEnableDebugRendering = true; });
    DebugConsole::registerCommand(
        "hideCollision", [this](GameInstance* pGameInstance) { bEnableDebugRendering = false; });
#endif
#endif
}

PhysicsManager::~PhysicsManager() {
    if (!simulatedBodies.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some simulated bodies registered");
    }
    if (!movingBodies.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some moving bodies registered");
    }
    if (!characterBodies.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some character bodies registered");
    }
    if (!bodyIdToPtr.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some alive bodies registered");
    }

    if (!pCharVsCharCollision->mCharacters.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "physics manager is being destroyed but there are still some characters registered");
    }

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsManager::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    // Ideally we should separate rendering and physics, have 60 tickrate for physics
    // and do physics interpolation in case the FPS is higher than 60 but because
    // physics interpolation is a pain in the back I go the stupid way:

    constexpr float MIN_UPDATE_TIME = 1.0F / 40.0F;

#if !defined(ENGINE_EDITOR)
    float deltaTimeLeftToSimulate = std::min(timeSincePrevFrameInSec, 2.0F * MIN_UPDATE_TIME);
    for (size_t i = 0; (i < 2) && (deltaTimeLeftToSimulate > 0.001F); i++) { // do at max 2 iterations
        PROFILE_SCOPE("physics tick")

        const float deltaTime = std::min(MIN_UPDATE_TIME, deltaTimeLeftToSimulate);
        deltaTimeLeftToSimulate -= deltaTime;

        {
            PROFILE_SCOPE("onBeforePhysicsUpdate - SimulatedBodyNode")
            for (const auto& pSimulatedNode : simulatedBodies) {
                pSimulatedNode->onBeforePhysicsUpdate(deltaTime);
            }
        }

        {
            PROFILE_SCOPE("onBeforePhysicsUpdate - MovingBodyNode")
            for (const auto& pMovingBody : movingBodies) {
#if defined(DEBUG)
                pMovingBody->bIsInPhysicsTick = true;
#endif
                pMovingBody->onBeforePhysicsUpdate(deltaTime);
#if defined(DEBUG)
                pMovingBody->bIsInPhysicsTick = false;
#endif
            }
        }

        {
            PROFILE_SCOPE("onBeforePhysicsUpdate - CharacterBodyNode")
            for (const auto& pCharacterBody : characterBodies) {
#if defined(DEBUG)
                pCharacterBody->bIsInPhysicsTick = true;
#endif
                pCharacterBody->onBeforePhysicsUpdate(deltaTime);
#if defined(DEBUG)
                pCharacterBody->bIsInPhysicsTick = false;
#endif
                pCharacterBody->updateCharacterPosition(*pPhysicsSystem, *pTempAllocator, deltaTime);
            }
        }

        {
            PROFILE_SCOPE("JPH::PhysicsSystem::Update")
            pPhysicsSystem->Update(deltaTime, 1, pTempAllocator.get(), pJobSystem.get());
        }

        // Update nodes according to simulation results.
        if (!simulatedBodies.empty()) {
            PROFILE_SCOPE("update simulated bodies after simulation")

            for (const auto& pSimulatedBodyNode : simulatedBodies) {
                JPH::Vec3 position{};
                JPH::Quat rotation{};
                pPhysicsSystem->GetBodyInterface().GetPositionAndRotation(
                    pSimulatedBodyNode->pBody->GetID(), position, rotation);

                pSimulatedBodyNode->setPhysicsSimulationResults(
                    convertPosDirFromJolt(position), convertRotationFromJolt(rotation));
            }
        }
        if (!movingBodies.empty()) {
            PROFILE_SCOPE("update moving bodies after simulation")

            for (const auto& pMovingBodyNode : movingBodies) {
                JPH::Vec3 position{};
                JPH::Quat rotation{};
                pPhysicsSystem->GetBodyInterface().GetPositionAndRotation(
                    pMovingBodyNode->pBody->GetID(), position, rotation);

                pMovingBodyNode->setPhysicsSimulationResults(
                    convertPosDirFromJolt(position), convertRotationFromJolt(rotation));
            }
        }

        {
            PROFILE_SCOPE("CharacterBodyNode::processContactEvents")
            for (const auto& pCharacterBody : characterBodies) {
                pCharacterBody->processContactEvents();
            }
        }

        // Process contacts.
        {
            auto& mtxWorlds = pGameManager->getWorlds();
            std::scoped_lock guardWorld(mtxWorlds.first);
            if (!mtxWorlds.second.vWorlds.empty()) {
                const auto pWorld = mtxWorlds.second.vWorlds[0].get();

                std::scoped_lock guardContacts(mtxContactData.first);

                // Prepare lambda to process contact events.
                const auto processContacts = [pWorld](std::queue<ContactInfo>& contacts) {
                    while (!contacts.empty()) {
                        auto& info = contacts.front();

                        // Get sensor node.
                        const auto pSensorNode = pWorld->getSpawnedNodeById(info.iSensorNodeId);
                        if (pSensorNode == nullptr) [[unlikely]] {
                            Error::showErrorAndThrowException(
                                "unable to determine contact node from body id");
                        }

                        // Cast type.
#if defined(DEBUG)
                        const auto pTriggerNode = dynamic_cast<TriggerVolumeNode*>(pSensorNode);
                        if (pTriggerNode == nullptr) [[unlikely]] {
                            Error::showErrorAndThrowException(std::format(
                                "expected the node \"{}\" to be a trigger volume node",
                                pSensorNode->getNodeName()));
                        }
#else
                        const auto pTriggerNode = reinterpret_cast<TriggerVolumeNode*>(pSensorNode);
#endif

                        // Get other node.
                        const auto pHitNode = pWorld->getSpawnedNodeById(info.iOtherNodeId);
                        if (pHitNode == nullptr) [[unlikely]] {
                            Error::showErrorAndThrowException(
                                "unable to determine contact node from body id");
                        }

                        // Notify.
                        if (info.bIsAdded) {
                            pTriggerNode->onContactAdded(
                                pHitNode, info.contactPointLocation, info.worldNormal);
                        } else {
                            pTriggerNode->onContactRemoved(pHitNode);
                        }

                        contacts.pop();
                    }
                };

                // First process removed contacts because when a character changes its shape (possible due to
                // crouching) in a single update we will receive 2 events (old shape removed and new shape
                // added, the order might be different) but because we give nodes to the user (in contact
                // callback) the events received by the user might look like this: "added node, tick, added
                // node (again, new shape), removed node (old shape), tick" but we want: "added node, tick,
                // removed node (old shape), added node (new shape), tick".
                processContacts(mtxContactData.second.newContactsRemoved);
                processContacts(mtxContactData.second.newContactsAdded);

                mtxContactData.second.newContactsAdded = {};
                mtxContactData.second.newContactsRemoved = {};
            }
        }
    }
#endif

#if defined(DEBUG)
    if (bEnableDebugRendering) {
        PROFILE_SCOPE("draw collision")

        JPH::BodyManager::DrawSettings drawSettings;
        pPhysicsSystem->DrawBodies(drawSettings, pPhysicsDebugDrawer.get());

        // Character bodies must be drawn explicitly.
        for (const auto& pCharacterBody : characterBodies) {
            pPhysicsDebugDrawer->DrawCapsule(
                pCharacterBody->pCharacterBody->GetCenterOfMassTransform(),
                pCharacterBody->pCollisionShape->getHalfHeight(),
                pCharacterBody->pCollisionShape->getRadius(),
                JPH::Color::sGrey,
                JPH::DebugRenderer::ECastShadow::Off,
                JPH::DebugRenderer::EDrawMode::Solid);
        }

        pPhysicsDebugDrawer->submitDrawData();
    }
#endif
}

JPH::Body* PhysicsManager::createBody(const JPH::BodyCreationSettings& settings) {
    JPH::Body* pCreatedBody = pPhysicsSystem->GetBodyInterface().CreateBody(settings);
    if (pCreatedBody == nullptr) {
        return nullptr;
    }

    const auto it = bodyIdToPtr.find(pCreatedBody->GetID());
    if (it != bodyIdToPtr.end()) [[unlikely]] {
        // We forgot somewhere to add/remove body from this map.
        Error::showErrorAndThrowException(
            "created a new body but a body with the same ID already exists in the alive bodies map");
    }
    bodyIdToPtr[pCreatedBody->GetID()] = pCreatedBody;

    return pCreatedBody;
}

void PhysicsManager::destroyBody(const JPH::BodyID& bodyId) {
    const auto it = bodyIdToPtr.find(bodyId);
    if (it == bodyIdToPtr.end()) [[unlikely]] {
        // We forgot somewhere to add/remove body from this map.
        Error::showErrorAndThrowException("unable to find body ID to destroy");
    }
    bodyIdToPtr.erase(it);

    pPhysicsSystem->GetBodyInterface().DestroyBody(bodyId);
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

    // Set node ID to body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }
    bodySettings.mUserData = *pNode->getNodeId();

    JPH::Body* pCreatedBody = createBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    if (pNode->isCollisionEnabled()) {
        // Add to physics world.
        pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::DontActivate);
    }

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

    // Collision nodes can temporary disable collision (removed from physics world) thus do a check:
    if (pPhysicsSystem->GetBodyInterface().IsAdded(pNode->pBody->GetID())) {
        // Remove from physics world.
        pPhysicsSystem->GetBodyInterface().RemoveBody(pNode->pBody->GetID());
    }

    // Destroy body.
    destroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;
}

void PhysicsManager::createBodyForNode(TriggerVolumeNode* pNode) {
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
    bodySettings.mIsSensor = true;

    // Set node ID to body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }
    bodySettings.mUserData = *pNode->getNodeId();

    JPH::Body* pCreatedBody = createBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    if (pNode->isTriggerEnabled()) {
        // Add to physics world.
        pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::Activate);
    }

    // Save created body.
    pNode->pBody = pCreatedBody;
}

void PhysicsManager::destroyBodyForNode(TriggerVolumeNode* pNode) {
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

    // Trigger volume nodes can temporary disable collision (removed from physics world) thus do a check:
    if (pPhysicsSystem->GetBodyInterface().IsAdded(pNode->pBody->GetID())) {
        // Remove from physics world.
        pPhysicsSystem->GetBodyInterface().RemoveBody(pNode->pBody->GetID());
    }

    // Destroy body.
    destroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;
}

void PhysicsManager::createBodyForNode(SimulatedBodyNode* pNode) {
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

    // Create body.
    JPH::BodyCreationSettings bodySettings(
        shapeResult.Get(),
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        JPH::EMotionType::Dynamic,
        static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));

    bodySettings.mFriction = pNode->friction;
    if (pNode->massKg > 0.0F) {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = pNode->massKg;
    }

    // Set node ID to body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }
    bodySettings.mUserData = *pNode->getNodeId();

    JPH::Body* pCreatedBody = createBody(bodySettings);
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
    if (!simulatedBodies.insert(pNode).second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was already registered as a simulated body", pNode->getNodeName()));
    }
}

void PhysicsManager::destroyBodyForNode(SimulatedBodyNode* pNode) {
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
    destroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;

    // Unregister.
    const auto it = simulatedBodies.find(pNode);
    if (it == simulatedBodies.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was not registered as a simulated body", pNode->getNodeName()));
    }
    simulatedBodies.erase(it);
}

void PhysicsManager::createBodyForNode(MovingBodyNode* pNode) {
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
        JPH::EMotionType::Kinematic,
        static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));

    // Set node ID to body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }
    bodySettings.mUserData = *pNode->getNodeId();

    JPH::Body* pCreatedBody = createBody(bodySettings);
    if (pCreatedBody == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "failed to create a physics body for the node \"{}\", probably run out of bodies",
            pNode->getNodeName()));
    }

    // Add to physics world.
    pPhysicsSystem->GetBodyInterface().AddBody(pCreatedBody->GetID(), JPH::EActivation::Activate);

    // Save created body.
    pNode->pBody = pCreatedBody;

    // Register.
    if (!movingBodies.insert(pNode).second) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was already registered as a moving body", pNode->getNodeName()));
    }
}

void PhysicsManager::destroyBodyForNode(MovingBodyNode* pNode) {
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
    destroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;

    // Unregister.
    const auto it = movingBodies.find(pNode);
    if (it == movingBodies.end()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("node \"{}\" was not registered as a moving body", pNode->getNodeName()));
    }
    movingBodies.erase(it);
}

void PhysicsManager::createBodyForNode(CompoundCollisionNode* pNode) {
    PROFILE_FUNC

    const auto mtxChildNodes = pNode->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);

    if (mtxChildNodes.second.empty()) {
        Log::warn(std::format(
            "expected the compound collision node \"{}\" to have child collision nodes",
            pNode->getNodeName()));
        return;
    }
    if (mtxChildNodes.second.size() == 1) {
        Log::warn(std::format(
            "compound collision node \"{}\" has only 1 child collision node, in this case it's better to "
            "create a single collision node instead of using a compound",
            pNode->getNodeName()));
    }

    JPH::Array<JPH::Ref<JPH::Shape>> vShapes;
    vShapes.reserve(mtxChildNodes.second.size());
    JPH::StaticCompoundShapeSettings compoundSettings;
    for (const auto& pChildNode : mtxChildNodes.second) {
        // Cast type.
        const auto pCollisionNode = dynamic_cast<CollisionNode*>(pChildNode);
        if (pCollisionNode == nullptr) [[unlikely]] {
            Log::error(std::format(
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

    // Set node ID to body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }
    bodySettings.mUserData = *pNode->getNodeId();

    JPH::Body* pCreatedBody = createBody(bodySettings);
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
    destroyBody(pNode->pBody->GetID());
    pNode->pBody = nullptr;
}

void PhysicsManager::createBodyForNode(CharacterBodyNode* pNode) {
    PROFILE_FUNC

    // Prepare settings.
    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = glm::radians(pNode->getMaxWalkSlopeAngle());
    settings->mShape = CharacterBodyNode::createAdjustedJoltShapeForCharacter(pNode->getBodyShape());
    settings->mSupportingVolume = JPH::Plane(
        convertPosDirToJolt(glm::vec3(Globals::WorldDirection::up)), -pNode->pCollisionShape->getRadius());
    settings->mEnhancedInternalEdgeRemoval = false;
    settings->mInnerBodyShape = settings->mShape;
    settings->mInnerBodyLayer = static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING);

    // Get node ID to set body's custom data.
    if (!pNode->getNodeId().has_value()) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format("expected the node \"{}\" to have ID initialized", pNode->getNodeName()));
    }

    // Create character.
    pNode->pCharacterBody = new JPH::CharacterVirtual(
        settings,
        convertPosDirToJolt(pNode->getWorldLocation()),
        convertRotationToJolt(pNode->getWorldRotation()),
        *pNode->getNodeId(),
        pPhysicsSystem.get());

    pNode->pCharacterBody->SetCharacterVsCharacterCollision(pCharVsCharCollision.get());
    if (pNode->pContactListener == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(std::format(
            "expected the contact listener on the node \"{}\" to be valid", pNode->getNodeName()));
    }
    pNode->pCharacterBody->SetListener(pNode->pContactListener.get());

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

std::optional<PhysicsManager::RayCastHit> PhysicsManager::castRayUntilHit(
    const glm::vec3& rayStartPosition,
    const glm::vec3& rayEndPosition,
    const std::vector<JPH::BodyID>& vIgnoredBodies) {
    PROFILE_FUNC

    const auto rayDirectionAndLength = rayEndPosition - rayStartPosition;

    // Prepare filters.
    const JPH::DefaultBroadPhaseLayerFilter broadPhaseLayerFilter =
        pPhysicsSystem->GetDefaultBroadPhaseLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));
    const JPH::DefaultObjectLayerFilter objectLayerFilter =
        pPhysicsSystem->GetDefaultLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));

    std::unique_ptr<JPH::BodyFilter> pBodyFilter = std::make_unique<JPH::BodyFilter>();
    if (vIgnoredBodies.size() == 1) {
        pBodyFilter = std::make_unique<JPH::IgnoreSingleBodyFilter>(vIgnoredBodies[0]);
    } else if (!vIgnoredBodies.empty()) {
        auto pFilter = std::make_unique<JPH::IgnoreMultipleBodiesFilter>();
        for (const auto& bodyId : vIgnoredBodies) {
            pFilter->IgnoreBody(bodyId);
        }
        pBodyFilter = std::move(pFilter);
    }

    // Cast ray.
    JPH::RRayCast ray(convertPosDirToJolt(rayStartPosition), convertPosDirToJolt(rayDirectionAndLength));
    JPH::RayCastResult result{};
    if (!pPhysicsSystem->GetNarrowPhaseQuery().CastRay(
            ray, result, broadPhaseLayerFilter, objectLayerFilter, *pBodyFilter)) {
        return {};
    }

    const auto bodyIt = bodyIdToPtr.find(result.mBodyID);
    if (bodyIt == bodyIdToPtr.end()) [[unlikely]] {
        // We forgot to add/remove body somewhere.
        Error::showErrorAndThrowException("unable to find body by ID");
    }
    const auto pHitBody = bodyIt->second;

    const auto hitPosition = rayStartPosition + rayDirectionAndLength * result.mFraction;
    const auto hitNormal = convertPosDirFromJolt(
        pHitBody->GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ray.GetPointOnRay(result.mFraction)));

    return RayCastHit{.bodyId = result.mBodyID, .hitPosition = hitPosition, .hitNormal = hitNormal};
}

std::vector<PhysicsManager::RayCastHit> PhysicsManager::castRayHitMultipleSort(
    const glm::vec3& rayStartPosition,
    const glm::vec3& rayEndPosition,
    const std::vector<JPH::BodyID>& vIgnoredBodies) {
    PROFILE_FUNC

    const auto rayDirectionAndLength = rayEndPosition - rayStartPosition;

    // Prepare filters.
    const JPH::DefaultBroadPhaseLayerFilter broadPhaseLayerFilter =
        pPhysicsSystem->GetDefaultBroadPhaseLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));
    const JPH::DefaultObjectLayerFilter objectLayerFilter =
        pPhysicsSystem->GetDefaultLayerFilter(static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING));

    std::unique_ptr<JPH::BodyFilter> pBodyFilter = std::make_unique<JPH::BodyFilter>();
    if (vIgnoredBodies.size() == 1) {
        pBodyFilter = std::make_unique<JPH::IgnoreSingleBodyFilter>(vIgnoredBodies[0]);
    } else if (!vIgnoredBodies.empty()) {
        auto pFilter = std::make_unique<JPH::IgnoreMultipleBodiesFilter>();
        for (const auto& bodyId : vIgnoredBodies) {
            pFilter->IgnoreBody(bodyId);
        }
        pBodyFilter = std::move(pFilter);
    }

    JPH::RayCastSettings settings;
    settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
    settings.mTreatConvexAsSolid = true;

    // Cast ray.
    JPH::RRayCast ray(convertPosDirToJolt(rayStartPosition), convertPosDirToJolt(rayDirectionAndLength));
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    pPhysicsSystem->GetNarrowPhaseQuery().CastRay(
        ray, settings, collector, broadPhaseLayerFilter, objectLayerFilter, *pBodyFilter);
    if (collector.mHits.empty()) {
        return {};
    }

    collector.Sort();

    std::vector<RayCastHit> vHits;
    vHits.reserve(collector.mHits.size());
    for (const auto& hitInfo : collector.mHits) {
        const auto bodyIt = bodyIdToPtr.find(hitInfo.mBodyID);
        if (bodyIt == bodyIdToPtr.end()) [[unlikely]] {
            // We forgot to add/remove body somewhere.
            Error::showErrorAndThrowException("unable to find body by ID");
        }
        const auto pHitBody = bodyIt->second;

        const auto hitPosition = rayStartPosition + rayDirectionAndLength * hitInfo.mFraction;
        const auto hitNormal = convertPosDirFromJolt(
            pHitBody->GetWorldSpaceSurfaceNormal(hitInfo.mSubShapeID2, ray.GetPointOnRay(hitInfo.mFraction)));

        vHits.push_back(
            RayCastHit{.bodyId = hitInfo.mBodyID, .hitPosition = hitPosition, .hitNormal = hitNormal});
    }

    return vHits;
}

void PhysicsManager::addRemoveBody(JPH::Body* pBody, bool bAdd, bool bActivate) {
    if (bAdd) {
        pPhysicsSystem->GetBodyInterface().AddBody(
            pBody->GetID(), bActivate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    } else {
        pPhysicsSystem->GetBodyInterface().RemoveBody(pBody->GetID());
    }
}

void PhysicsManager::setBodyLocationRotation(
    JPH::Body* pBody, const glm::vec3& location, const glm::vec3& rotation) {
    pPhysicsSystem->GetBodyInterface().SetPositionAndRotation(
        pBody->GetID(),
        convertPosDirToJolt(location),
        convertRotationToJolt(rotation),
        JPH::EActivation::DontActivate);
}

void PhysicsManager::setBodyActiveState(JPH::Body* pBody, bool bActivate) {
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

void PhysicsManager::moveKinematic(
    JPH::Body* pBody, const glm::vec3& worldLocation, const glm::vec3& worldRotation, float deltaTime) {
    pPhysicsSystem->GetBodyInterface().MoveKinematic(
        pBody->GetID(), convertPosDirToJolt(worldLocation), convertRotationToJolt(worldRotation), deltaTime);
}

void PhysicsManager::setLinearVelocity(JPH::Body* pBody, const glm::vec3& velocity) {
    pPhysicsSystem->GetBodyInterface().SetLinearVelocity(pBody->GetID(), convertPosDirToJolt(velocity));
}

void PhysicsManager::setAngularVelocity(JPH::Body* pBody, const glm::vec3& velocity) {
    pPhysicsSystem->GetBodyInterface().SetAngularVelocity(pBody->GetID(), convertPosDirToJolt(velocity));
}

bool PhysicsManager::isBodySensor(JPH::BodyID bodyId) {
    return pPhysicsSystem->GetBodyInterface().IsSensor(bodyId);
}

glm::vec3 PhysicsManager::getLinearVelocity(JPH::Body* pBody) {
    return convertPosDirFromJolt(pPhysicsSystem->GetBodyInterface().GetLinearVelocity(pBody->GetID()));
}

glm::vec3 PhysicsManager::getAngularVelocity(JPH::Body* pBody) {
    return convertPosDirFromJolt(pPhysicsSystem->GetBodyInterface().GetAngularVelocity(pBody->GetID()));
}

uint64_t PhysicsManager::getUserDataFromBody(JPH::BodyID bodyId) {
    return pPhysicsSystem->GetBodyInterface().GetUserData(bodyId);
}

void PhysicsManager::optimizeBroadPhase() {
    PROFILE_FUNC

    pPhysicsSystem->OptimizeBroadPhase();
}

glm::vec3 PhysicsManager::getGravity() { return convertPosDirFromJolt(pPhysicsSystem->GetGravity()); }

JPH::PhysicsSystem& PhysicsManager::getPhysicsSystem() { return *pPhysicsSystem; }

JPH::TempAllocator& PhysicsManager::getTempAllocator() { return *pTempAllocator; }
