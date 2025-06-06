#pragma once

// Standard.
#include <filesystem>
#include <optional>
#include <functional>

// Custom.
#include "misc/Error.h"

/**
 * Provides static functions for importing files in special formats (such as GLTF/GLB) as meshes,
 * textures, etc. into engine formats (such as nodes).
 */
class GltfImporter {
public:
    GltfImporter() = delete;

    /**
     * Imports a file in a special format (such as GTLF/GLB) and converts information
     * from the file to a new node tree with materials and textures (if the specified file defines them).
     *
     * @param pathToFile                  Path to the file to import.
     * @param sPathToOutputDirRelativeRes Path to a directory relative to the `res` directory that will
     * store results, for example: `game/models` (located at `res/game/models`).
     * @param sOutputDirectoryName        Name of the new directory that does not exists yet but
     * will be created in the specified directory (relative to the `res`) to store the results
     * (allowed characters A-z and numbers 0-9, maximum length is 10 characters), for example: `mesh`.
     * @param onProgress                  Callback that will be called to report some text description of
     * the current import stage.
     *
     * @return Error if something went wrong.
     */
    [[nodiscard]] static std::optional<Error> importFileAsNodeTree(
        const std::filesystem::path& pathToFile,
        const std::string& sPathToOutputDirRelativeRes,
        const std::string& sOutputDirectoryName,
        const std::function<void(std::string_view)>& onProgress);
};
