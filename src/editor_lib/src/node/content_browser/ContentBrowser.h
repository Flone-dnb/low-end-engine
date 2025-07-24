#pragma once

// Standard.
#include <filesystem>
#include <unordered_set>

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;

/** Displays filesystem. */
class ContentBrowser : public RectUiNode {
public:
    ContentBrowser();

    /**
     * Creates a new node with the specified name.
     *
     * @param sNodeName Name of this node.
     */
    ContentBrowser(const std::string& sNodeName);

    virtual ~ContentBrowser() override = default;

    /** Rebuilds displayed entires. */
    void rebuildFileTree();

private:
    /**
     * Creates a context menu for right click on a directory.
     *
     * @param pathToDirectory Path to the directory that was clicked.
     */
    void showDirectoryContextMenu(const std::filesystem::path& pathToDirectory);

    /**
     * Creates a context menu for right click on a file.
     *
     * @param pathToFile Path to the file that was clicked.
     */
    void showFileContextMenu(const std::filesystem::path& pathToFile);

    /**
     * Adds filesystem entires.
     *
     * @param pathToDirectory Path to start recursively checking.
     * @param iNesting        Used for recursion.
     */
    void displayDirectoryRecursive(const std::filesystem::path& pathToDirectory, size_t iNesting);

    /**
     * Displays file or a directory.
     *
     * @param entry File or a directory.
     * @param iNesting Nesting for text.
     */
    void displayFilesystemEntry(const std::filesystem::path& entry, size_t iNesting);

    /** Paths to expanded directories. */
    std::unordered_set<std::filesystem::path> openedDirectoryPaths;

    /** Layout to add file and directory entries. */
    LayoutUiNode* pResContentLayout = nullptr;
};
