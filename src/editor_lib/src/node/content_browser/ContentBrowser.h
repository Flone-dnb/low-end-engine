#pragma once

// Standard.
#include <unordered_set>

// Custom.
#include "game/node/ui/RectUiNode.h"

class LayoutUiNode;

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

private:
    /**
     * Creates a context menu for right click on a directory.
     *
     * @param pathToDirectory Path to the directory that was clicked.
     */
    void onDirectoryRightClick(const std::filesystem::path& pathToDirectory);

    /** Rebuilds displayed entires. */
    void rebuildFileTree();

    /** Paths to expanded directories. */
    std::unordered_set<std::filesystem::path> openedDirectoryPaths;

    /** Layout to add file and directory entries. */
    LayoutUiNode* pResContentLayout = nullptr;
};
