#include "game/geometry/shapes/Frustum.h"

Frustum Frustum::create(
    const glm::vec3& cameraPosition,
    const glm::vec3& forwardDirection,
    const glm::vec3& upDirection,
    float nearClipPlaneDistance,
    float fapClipPlaneDistance,
    float verticalFovInRadians,
    float aspectRatio) {
    // Precalculate `tan(fov/2)` because we will need it multiple times.
    // By using the following rule: tan(X) = opposite side / adjacent side
    // this value gives us: far clip plane half height / z far
    //                  /|
    //                 / |
    //                /  |  <- camera frustum from side view (not top view)
    //               /   |
    // camera:   fov ----- zFar
    //               \   |
    //                \  |  <- frustum half height
    //                 \ |
    //                  \|
    const auto tanHalfFov = std::tan(0.5f * verticalFovInRadians);
    const auto farClipPlaneHalfHeight = fapClipPlaneDistance * tanHalfFov;
    const auto farClipPlaneHalfWidth = farClipPlaneHalfHeight * aspectRatio;
    const auto rightDirection = glm::normalize(glm::cross(forwardDirection, upDirection));
    const auto toFarPlaneRelativeLocation = fapClipPlaneDistance * forwardDirection;

    Frustum frustum;

    frustum.nearFace = Plane(forwardDirection, cameraPosition + nearClipPlaneDistance * forwardDirection);

    frustum.farFace = Plane(-forwardDirection, cameraPosition + toFarPlaneRelativeLocation);

    frustum.rightFace = Plane(
        glm::normalize(
            glm::cross(upDirection, toFarPlaneRelativeLocation + rightDirection * farClipPlaneHalfWidth)),
        cameraPosition);

    frustum.leftFace = Plane(
        glm::normalize(
            glm::cross(toFarPlaneRelativeLocation - rightDirection * farClipPlaneHalfWidth, upDirection)),
        cameraPosition);

    frustum.topFace = Plane(
        glm::normalize(
            glm::cross(toFarPlaneRelativeLocation + upDirection * farClipPlaneHalfHeight, rightDirection)),
        cameraPosition);

    frustum.bottomFace = Plane(
        glm::normalize(
            glm::cross(rightDirection, toFarPlaneRelativeLocation - upDirection * farClipPlaneHalfHeight)),
        cameraPosition);

    return frustum;
}
