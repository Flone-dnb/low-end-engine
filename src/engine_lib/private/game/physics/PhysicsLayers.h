#pragma once

// External.
#include "Jolt/Jolt.h" // Always include Jolt.h before including any other Jolt header.
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

/**
 * Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to
 * have a layer for non-moving and moving objects to avoid having to update a tree full of static objects
 * every frame. You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case)
 * but if you have many object layers you'll be creating many broad phase trees, which is not efficient. If
 * you want to fine tune your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats
 * reported on the TTY.
 */
enum class BroadPhaseLayer : JPH::BroadPhaseLayer::Type {
    NON_MOVING = 0,
    MOVING,
    // ... new layers go here ...

    COUNT, // defines the total number of elements in this enum
};

/**
 * Layer that objects can be in, determines which other objects it can collide with
 * Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can
 * have more layers if you want. E.g. you could have a layer for high detail collision (which is not used by
 * the physics simulation but only if you do collision testing).
 */
enum class ObjectLayer : JPH::ObjectLayer {
    NON_MOVING = 0,
    MOVING,
    // ... new layers go here ...

    COUNT, // defines the total number of elements in this enum
};

/** Determines if two object layers can collide. */
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    /**
     * @param layer1Index Layer 1.
     * @param layer2Index Layer 2.
     *
     * @return Should collide.
     */
    virtual bool ShouldCollide(JPH::ObjectLayer layer1Index, JPH::ObjectLayer layer2Index) const override;
};

/** Determines if an object layer can collide with a broadphase layer. */
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    /**
     * @param objectLayer Object layer.
     * @param broadPhaseLayer Broad phase layer.
     *
     * @return Should collide.
     */
    virtual bool
    ShouldCollide(JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const override;
};

/** This defines a mapping between object and broadphase layers. */
class BroadPhaseLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BroadPhaseLayerInterfaceImpl();

    /**
     * @return Total number of broad layers.
     */
    virtual unsigned int GetNumBroadPhaseLayers() const override;

    /**
     * @param objectLayerIndex Object layer index.
     * @return Layer.
     */
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer objectLayerIndex) const override;

private:
    /** Object to broad phase layer mapping. */
    JPH::BroadPhaseLayer objectLayerToBroadPhaseLayer[static_cast<unsigned int>(ObjectLayer::COUNT)];
};
