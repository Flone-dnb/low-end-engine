#include "game/physics/PhysicsManager.h"

// Standard.
#include <thread>

// Custom.
#include "misc/Error.h"
#include "misc/Profiler.hpp"
#include "game/physics/PhysicsLayers.h"
#include "game/physics/CoordinateConversions.hpp"
#include "game/node/physics/CollisionNode.h"
#include "game/geometry/shapes/CollisionShape.h"
#include "render/PhysicsDebugDrawer.hpp"
#include "game/DebugConsole.h"

#if defined(__aarch64__) || defined(__ARM64__)
#define IS_ARM64
#endif

// External.
#ifndef IS_ARM64
#include "immintrin.h"
#include "cpuid.h"
#endif
#include "Jolt/Jolt.h" // Always include Jolt.h before including any other Jolt header.
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"

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
#if defined(WIN32)
    __cpuid(cpuInfo, 0x80000001);
    const bool lzcntSupported = (cpuInfo[2] & (1 << 5)) != 0;
#else
    __cpuid_count(0x80000001, 0, eax, ebx, ecx, edx);
    const bool lzcntSupported = (ecx & (1 << 5)) != 0;
#endif

    // TZCNT
#if defined(WIN32)
    __cpuidex(cpuInfo, 7, 0);
    const bool tzcntSupported = (cpuInfo[1] & (1 << 3)) != 0;
#else
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    const bool tzcntSupported = (ebx & (1 << 3)) != 0;
#endif

    // F16C
#if defined(WIN32)
    __cpuid(cpuInfo, 1);
    const bool f16cSupported = (cpuInfo[2] & (1 << 29)) != 0;
#else
    __cpuid_count(1, 0, eax, ebx, ecx, edx);
    const bool f16cSupported = (ecx & (1 << 29)) != 0;
#endif

    // SSE4.2
#if defined(WIN32)
    const bool sse42Supported = __check_isa_support(__IA_SUPPORT_SSE42);
#else
    __cpuid(1, eax, ebx, ecx, edx);
    const bool sse42Supported = (ecx & (1 << 20)) != 0;
#endif

    if (!sse42Supported || !lzcntSupported || !tzcntSupported || !f16cSupported) {
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

    pTempAllocator = std::make_unique<JPH::TempAllocatorImpl>(1024 * 512); // 512 KB

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
    pPhysicsSystem->SetGravity(convertToJolt(glm::vec3(0.0F, 0.0F, -9.81F)));

#if defined(DEBUG)
    pPhysicsDebugDrawer = std::make_unique<PhysicsDebugDrawer>();

#if defined(ENGINE_EDITOR)
    bEnableDebugRendering = true;
#else
    DebugConsole::get().registerCommand(
        "showPhysicsBodies", [this](GameInstance* pGameInstance) { bEnableDebugRendering = true; });
    DebugConsole::get().registerCommand(
        "hidePhysicsBodies", [this](GameInstance* pGameInstance) { bEnableDebugRendering = false; });
#endif
#endif
}

PhysicsManager::~PhysicsManager() {
    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsManager::onBeforeNewFrame(float timeSincePrevFrameInSec) {
    PROFILE_FUNC

    constexpr float JOLT_UPDATE_DELTA_TIME = 1.0F / 60.0F;

    // TODO: run physics tick

    // Simulate Jolt.
    timeSinceLastJoltUpdate += timeSincePrevFrameInSec;
    while (timeSinceLastJoltUpdate >= JOLT_UPDATE_DELTA_TIME) {
        timeSinceLastJoltUpdate -= JOLT_UPDATE_DELTA_TIME;

        pPhysicsSystem->Update(JOLT_UPDATE_DELTA_TIME, 1, pTempAllocator.get(), pJobSystem.get());
    };

#if defined(DEBUG)
    if (bEnableDebugRendering) {
        JPH::BodyManager::DrawSettings drawSettings;
        pPhysicsSystem->DrawBodies(drawSettings, pPhysicsDebugDrawer.get());
    }
#endif
}

#if defined(DEBUG)
void PhysicsManager::setEnableDebugRendering(bool bEnable) { bEnableDebugRendering = bEnable; }
#endif

void PhysicsManager::createBodyForNode(CollisionNode* pNode) {
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
        convertToJolt(pNode->getWorldLocation()),
        convertToJolt(glm::quat(pNode->getWorldRotation())),
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

void PhysicsManager::setBodyLocationRotation(
    JPH::Body* pBody, const glm::vec3& location, const glm::vec3& rotation) {
    pPhysicsSystem->GetBodyInterface().SetPositionAndRotation(
        pBody->GetID(),
        convertToJolt(location),
        convertToJolt(glm::quat(rotation)),
        JPH::EActivation::DontActivate);
}
