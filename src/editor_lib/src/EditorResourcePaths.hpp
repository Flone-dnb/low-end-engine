#pragma once

// Standard.
#include <string>
#include <filesystem>

// Custom.
#include "misc/ProjectPaths.h"

/** Provides paths to editor resources (shaders, models, etc). */
class EditorResourcePaths {
public:
    /**
     * Returns path to the directory that stores editor's models.
     *
     * @return Path to a directory.
     */
    static std::filesystem::path getPathToModelsDirectory() {
        return ProjectPaths::getPathToResDirectory(ResourceDirectory::EDITOR) / "models/";
    }

    /**
     * Returns path to a directory (relative to the `res` directory) that stores editor's shaders.
     *
     * @return Relative path to a directory.
     */
    static std::string getPathToShadersRelativeRes() { return "editor/shaders/"; }
};
