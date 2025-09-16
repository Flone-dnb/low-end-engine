#pragma once

// Standard.
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

// Custom.
#include "game/node/ui/RectUiNode.h"

class TextUiNode;
class LayoutUiNode;

/**
 * Requires a file or a directory to be selected.
 * Automatically detaches and despawns when closed.
 */
class FileDialogMenu : public RectUiNode {
public:
    FileDialogMenu() = delete;

    /**
     * Creates a new node.
     *
     * @param pathToDirectory Path to directory to show at start.
     * @param vFileExtensions Specify empty to accept any files or extensions
     * to only accept (for example ".gltf" for "*.gltf" files).
     * @param onSelected      Callback that will be called after the path is selected.
     */
    FileDialogMenu(
        const std::filesystem::path& pathToDirectory,
        const std::vector<std::string>& vFileExtensions,
        const std::function<void(const std::filesystem::path& path)>& onSelected);

    virtual ~FileDialogMenu() override = default;

protected:
    /**
     * Called after @ref onSpawning when this node and all of node's child nodes (at the moment
     * of spawning) were spawned.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark Generally you might want to prefer to use @ref onSpawning, this function
     * is mostly used to do some logic related to child nodes after all child nodes were spawned
     * (for example if you have a camera child node you can make it active in this function).=
     */
    virtual void onChildNodesSpawned() override;

    /**
     * Called before this node is despawned from the world to execute custom despawn logic.
     *
     * @warning If overriding you must call the parent's version of this function first
     * (before executing your logic) to execute parent's logic.
     *
     * @remark This node will be marked as despawned after this function is called.
     * @remark This function is called after all child nodes were despawned.
     * @remark @ref getSpawnDespawnMutex is locked while this function is called.
     */
    virtual void onDespawning() override;

private:
    /**
     * Clears previously shown content and shows contents of the specified directory.
     *
     * @param pathToDirectory Directory to show.
     */
    void showDirectory(std::filesystem::path pathToDirectory);

    /** Callback to call once the path is selected. */
    const std::function<void(const std::filesystem::path& path)> onSelected;

    /** Path to the currently shown directory. */
    std::filesystem::path pathToCurrentDirectory;

    /** Empty to accept any files or extensions to only accept (for example ".gltf" for "*.gltf" files). */
    const std::vector<std::string> vFileExtensions;

    /** Text that displays the current path. */
    TextUiNode* pCurrentPathText = nullptr;

    /** Layout to add directory entries. */
    LayoutUiNode* pFilesystemLayout = nullptr;
};
