#include "game/physics/PhysicsLayers.h"

// Custom.
#include "misc/Error.h"

BroadPhaseLayerInterfaceImpl::BroadPhaseLayerInterfaceImpl() {
    // Create a mapping table from object layer to broad phase layer.

    objectLayerToBroadPhaseLayer[static_cast<unsigned int>(ObjectLayer::NON_MOVING)] =
        JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayer::NON_MOVING));

    objectLayerToBroadPhaseLayer[static_cast<unsigned int>(ObjectLayer::MOVING)] =
        JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayer::MOVING));
}

unsigned int BroadPhaseLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
    return static_cast<unsigned int>(BroadPhaseLayer::COUNT);
}

JPH::BroadPhaseLayer
BroadPhaseLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer objectLayerIndex) const {
    if (objectLayerIndex >= static_cast<JPH::ObjectLayer>(ObjectLayer::COUNT)) [[unlikely]] {
        Error::showErrorAndThrowException("layer index out of bounds");
    }
    return objectLayerToBroadPhaseLayer[objectLayerIndex];
}

bool ObjectLayerPairFilterImpl::ShouldCollide(
    JPH::ObjectLayer layer1Index, JPH::ObjectLayer layer2Index) const {
    switch (layer1Index) {
    case static_cast<JPH::ObjectLayer>(ObjectLayer::NON_MOVING): {
        // Non moving only collides with moving.
        return layer2Index == static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING);
    }
    case static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING): {
        // Moving collides with everything.
        return true;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
        break;
    }
    }
}

bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(
    JPH::ObjectLayer objectLayer, JPH::BroadPhaseLayer broadPhaseLayer) const {
    switch (objectLayer) {
    case static_cast<JPH::ObjectLayer>(ObjectLayer::NON_MOVING): {
        // Non moving only collides with moving.
        return broadPhaseLayer ==
               JPH::BroadPhaseLayer(static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayer::MOVING));
    }
    case static_cast<JPH::ObjectLayer>(ObjectLayer::MOVING): {
        // Moving collides with everything.
        return true;
    }
    default: {
        Error::showErrorAndThrowException("unhandled case");
        break;
    }
    }
}
