#pragma once

// Standard.
#include <filesystem>
#include <vector>

// Custom.
#include "math/GLMath.hpp"

/** Stores array of positions to be converted to a convex shape for physics. */
class ConvexShapeGeometry {
public:
    ConvexShapeGeometry() = default;
    ~ConvexShapeGeometry() = default;

    /** Copy constructor. */
    ConvexShapeGeometry(const ConvexShapeGeometry&) = default;
    /** Copy assignment. @return this. */
    ConvexShapeGeometry& operator=(const ConvexShapeGeometry&) = default;

    /** Move constructor. */
    ConvexShapeGeometry(ConvexShapeGeometry&&) noexcept = default;
    /** Move assignment. @return this. */
    ConvexShapeGeometry& operator=(ConvexShapeGeometry&&) noexcept = default;

    /**
     * Deserializes the geometry from the file (also see @ref serialize).
     *
     * @param pathToFile File to deserialize from.
     *
     * @return Geometry.
     */
    static ConvexShapeGeometry deserialize(const std::filesystem::path& pathToFile);

    /**
     * Serializes the geometry data into a file.
     *
     * @param pathToFile File to serialize to.
     */
    void serialize(const std::filesystem::path& pathToFile) const;

    /**
     * Returns shape positions (vertices).
     *
     * @return Positions.
     */
    std::vector<glm::vec3>& getPositions() { return vPositions; }

    /**
     * Returns shape positions (vertices).
     *
     * @return Positions.
     */
    const std::vector<glm::vec3>& getPositions() const { return vPositions; }

private:
    /** Positions (vertices) of the convex mesh. */
    std::vector<glm::vec3> vPositions;
};
