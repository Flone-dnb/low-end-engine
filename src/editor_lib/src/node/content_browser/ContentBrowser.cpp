#include "node/content_browser/ContentBrowser.h"

// Standard.
#include <fstream>
#include <filesystem>

// Custom.
#include "misc/ProjectPaths.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "EditorTheme.h"
#include "EditorGameInstance.h"
#include "node/menu/SetNameMenu.h"
#include "node/menu/FileDialogMenu.h"
#include "material/TextureManager.h"
#include "node/menu/ConfirmationMenu.h"
#include "io/GltfImporter.h"

// External.
#include "utf/utf.hpp"

ContentBrowser::ContentBrowser() : ContentBrowser("Content Browser") {}
ContentBrowser::ContentBrowser(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setColor(EditorTheme::getContainerBackgroundColor());
    setPadding(EditorTheme::getPadding() / 2.0F);

    pResContentLayout = addChildNode(std::make_unique<LayoutUiNode>());
    pResContentLayout->setIsScrollBarEnabled(true);
    pResContentLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pResContentLayout->setPadding(EditorTheme::getPadding());

    rebuildFileTree();
}

void ContentBrowser::rebuildFileTree() {
    {
        const auto mtxChildNodes = pResContentLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            pNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    const auto pathToGameRes = ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME, true);
    openedDirectoryPaths.insert(pathToGameRes);
    displayFilesystemEntry(pathToGameRes, 0);
}

void ContentBrowser::displayDirectoryRecursive(
    const std::filesystem::path& pathToDirectory, size_t iNesting) {
    if (std::filesystem::is_empty(pathToDirectory)) {
        std::string sNestingText;
        for (size_t i = 0; i < iNesting; i++) {
            sNestingText += "    ";
        }
        const auto pText = pResContentLayout->addChildNode(std::make_unique<TextUiNode>());
        pText->setTextHeight(EditorTheme::getTextHeight());
        pText->setText(utf::as_u16(sNestingText + "    empty"));
    }

    for (const auto& entry : std::filesystem::directory_iterator(pathToDirectory)) {
        displayFilesystemEntry(entry.path(), iNesting + 1);
    }
}

void ContentBrowser::displayFilesystemEntry(const std::filesystem::path& entry, size_t iNesting) {
    std::string sNestingText;
    for (size_t i = 0; i < iNesting; i++) {
        sNestingText += "    ";
    }

    std::string sEntryName;

    const auto pButton = pResContentLayout->addChildNode(std::make_unique<ButtonUiNode>());
    pButton->setPadding(EditorTheme::getPadding());
    pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
    pButton->setColor(EditorTheme::getButtonColor());
    pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
    pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
    if (std::filesystem::is_directory(entry)) {
        const auto dirOpenIt = openedDirectoryPaths.find(entry);
        const auto bIsDirOpened = dirOpenIt != openedDirectoryPaths.end();

        if (bIsDirOpened) {
            sEntryName = sNestingText + "[-] " + entry.filename().string();
        } else {
            sEntryName = sNestingText + "[/] " + entry.filename().string();
        }

        pButton->setOnClicked([this, pathToDir = entry]() {
            const auto it = openedDirectoryPaths.find(pathToDir);
            if (it == openedDirectoryPaths.end()) {
                openedDirectoryPaths.insert(pathToDir);
            } else {
                openedDirectoryPaths.erase(pathToDir);
            }
            rebuildFileTree();
        });
        pButton->setOnRightClick([this, entry]() { showDirectoryContextMenu(entry); });

        if (bIsDirOpened) {
            displayDirectoryRecursive(entry, iNesting);
        }
    } else {
        sEntryName = sNestingText + entry.filename().string();
        pButton->setOnClicked([this, pathToFile = entry]() {
            if (pathToFile.extension() != ".toml") {
                return;
            }
            const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
            if (pGameInstance == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected editor game instance to be valid");
            }
            pGameInstance->openNodeTreeAsGameWorld(pathToFile);
        });
        pButton->setOnRightClick([this, entry]() { showFileContextMenu(entry); });
    }
    {
        const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
        pText->setTextHeight(EditorTheme::getTextHeight());
        pText->setText(utf::as_u16(sEntryName));
    }
}

void ContentBrowser::showDirectoryContextMenu(const std::filesystem::path& pathToDirectory) {
    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance to be valid");
    }

    if (pGameInstance->isContextMenuOpened()) {
        // Already opened.
        return;
    }

    std::vector<std::pair<std::u16string, std::function<void()>>> vOptions;
    {
        vOptions.push_back(
            {u"Create node tree", [this, pathToDirectory]() {
                 const auto pSetNameMenu =
                     getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<SetNameMenu>());
                 pSetNameMenu->setOnNameChanged([this, pathToDirectory](std::u16string_view sText) {
                     const auto pathToNodeTree = pathToDirectory / utf::as_str8(sText);
                     if (std::filesystem::exists(pathToNodeTree)) {
                         return;
                     }

                     const auto pNode = std::make_unique<Node>("Root node");
                     const auto optionalError = pNode->serializeNodeTree(pathToNodeTree, false);
                     if (optionalError.has_value()) [[unlikely]] {
                         Log::error(std::format(
                             "failed to serialize new node tree, error: {}",
                             optionalError->getInitialMessage()));
                         return;
                     }

                     openedDirectoryPaths.insert(pathToDirectory);
                     rebuildFileTree();
                 });
             }});
        vOptions.push_back(
            {u"Create directory", [this, pathToDirectory]() {
                 const auto pSetNameMenu =
                     getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<SetNameMenu>());
                 pSetNameMenu->setOnNameChanged([this, pathToDirectory](std::u16string_view sText) {
                     const auto pathToNewDirectory = pathToDirectory / utf::as_str8(sText);
                     if (std::filesystem::exists(pathToNewDirectory)) {
                         return;
                     }
                     try {
                         std::filesystem::create_directory(pathToNewDirectory);
                     } catch (std::exception& exception) {
                         Log::error(
                             std::format("unable to create directory, error: {}", exception.what()));
                     }
                     openedDirectoryPaths.insert(pathToDirectory);
                     rebuildFileTree();
                 });
             }});
        vOptions.push_back(
            {u"Rename directory", [this, pathToDirectory]() {
                 const auto pSetNameMenu =
                     getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<SetNameMenu>());
                 pSetNameMenu->setOnNameChanged([this, pathToDirectory](std::u16string_view sText) {
                     const auto pathToNewDirectory = pathToDirectory.parent_path() / utf::as_str8(sText);
                     try {
                         std::filesystem::rename(pathToDirectory, pathToNewDirectory);
                     } catch (std::exception& exception) {
                         Log::error(
                             std::format("unable to rename directory, error: {}", exception.what()));
                     }
                     rebuildFileTree();
                 });
             }});
        vOptions.push_back(
            {u"Import .gltf/.glb", [this, pathToDirectory]() {
                 getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<FileDialogMenu>(
                     ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME),
                     std::vector<std::string>{".gltf", ".glb"},
                     [this, pathToDirectory](const std::filesystem::path& selectedPath) {
                         // Do async import to view import progress (messages in the log)
                         // and don't block the whole UI while importing large files.
                         getGameInstanceWhileSpawned()->addTaskToThreadPool(
                             [this, selectedPath, pathToDirectory]() {
                                 const auto sRelativeOutputPath =
                                     std::filesystem::relative(
                                         pathToDirectory,
                                         ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT))
                                         .string();

                                 const auto optionalError = GltfImporter::importFileAsNodeTree(
                                     selectedPath,
                                     sRelativeOutputPath,
                                     selectedPath.stem().string(),
                                     [](std::string_view sMessage) { Log::info(sMessage); });
                                 if (optionalError.has_value()) [[unlikely]] {
                                     Log::error(std::format(
                                         "failed to import the file, error: {}",
                                         optionalError->getInitialMessage()));
                                 } else {
                                     Log::info(std::format(
                                         "file \"{}\" was successfully imported",
                                         selectedPath.filename().string()));
                                     rebuildFileTree();
                                 }
                             });
                     }));
             }});
        vOptions.push_back(
            {u"Import collision shape", [this, pathToDirectory]() {
                 getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<FileDialogMenu>(
                     ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME),
                     std::vector<std::string>{".gltf", ".glb"},
                     [this, pathToDirectory](const std::filesystem::path& selectedPath) {
                         const auto optionalError = GltfImporter::importFileAsConvexShapeGeometry(
                             selectedPath, pathToDirectory, [](std::string_view sMessage) {
                                 Log::info(sMessage);
                             });
                         if (optionalError.has_value()) [[unlikely]] {
                             Log::error(std::format(
                                 "failed to import the file, error: {}", optionalError->getInitialMessage()));
                         } else {
                             Log::info(std::format(
                                 "file \"{}\" was successfully imported", selectedPath.filename().string()));
                             rebuildFileTree();
                         }
                     }));
             }});
        if (!pathToDirectory.string().ends_with("res/game")) {
            vOptions.push_back({u"Delete directory", [this, pathToDirectory]() {
                                    // Show confirmation.
                                    getWorldRootNodeWhileSpawned()->addChildNode(
                                        std::make_unique<ConfirmationMenu>(
                                            std::format("Delete \"{}\"?", pathToDirectory.stem().string()),
                                            [this, pathToDirectory]() {
                                                // Delete.
                                                openedDirectoryPaths.erase(pathToDirectory);
                                                std::filesystem::remove_all(pathToDirectory);
                                                rebuildFileTree();
                                            }));
                                }});
        }
    }
    dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())->openContextMenu(vOptions);
}

void ContentBrowser::showFileContextMenu(const std::filesystem::path& pathToFile) {
    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance to be valid");
    }

    if (pGameInstance->isContextMenuOpened()) {
        // Already opened.
        return;
    }

    std::vector<std::pair<std::u16string, std::function<void()>>> vOptions;
    {
        vOptions.push_back({u"Rename file", [this, pathToFile]() {
                                const auto pSetNameMenu = getWorldRootNodeWhileSpawned()->addChildNode(
                                    std::make_unique<SetNameMenu>());
                                pSetNameMenu->setOnNameChanged([this, pathToFile](std::u16string_view sText) {
                                    const auto pathToNewFile = pathToFile.parent_path() / utf::as_str8(sText);
                                    try {
                                        std::filesystem::rename(pathToFile, pathToNewFile);
                                    } catch (std::exception& exception) {
                                        Log::error(std::format(
                                            "unable to rename file, error: {}", exception.what()));
                                    }
                                    rebuildFileTree();
                                });
                            }});
        vOptions.push_back(
            {u"Delete file", [this, pathToFile]() {
                 // Show confirmation.
                 getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<ConfirmationMenu>(
                     std::format("Delete \"{}\"?", pathToFile.filename().string()), [this, pathToFile]() {
                         // Delete.
                         std::filesystem::remove(pathToFile);
                         rebuildFileTree();
                     }));
             }});
    }
    dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())->openContextMenu(vOptions);
}
