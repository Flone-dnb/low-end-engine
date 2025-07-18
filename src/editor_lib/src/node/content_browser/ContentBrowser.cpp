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

    const auto pathToResGameDirectory = ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME, true);

    const auto pButton = pResContentLayout->addChildNode(std::make_unique<ButtonUiNode>());
    pButton->setPadding(EditorTheme::getPadding());
    pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
    pButton->setColor(EditorTheme::getButtonColor());
    pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
    pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
    pButton->setOnRightClick(
        [this, pathToResGameDirectory]() { onDirectoryRightClick(pathToResGameDirectory); });
    {
        const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
        pText->setTextHeight(EditorTheme::getTextHeight());
        pText->setText(u"[-] res/game");
    }

    for (const auto& entry : std::filesystem::directory_iterator(pathToResGameDirectory)) {
        std::string sEntryName;

        const auto pButton = pResContentLayout->addChildNode(std::make_unique<ButtonUiNode>());
        pButton->setPadding(EditorTheme::getPadding());
        pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
        pButton->setColor(EditorTheme::getButtonColor());
        pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
        pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
        if (std::filesystem::is_directory(entry)) {
            sEntryName = "    [/] " + entry.path().filename().string();
            pButton->setOnClicked(
                [this, pathToDir = entry.path()]() { openedDirectoryPaths.insert(pathToDir); });
            pButton->setOnRightClick(
                [this, pathToResGameDirectory]() { onDirectoryRightClick(pathToResGameDirectory); });
        } else {
            sEntryName = "    " + entry.path().filename().string();
            pButton->setOnClicked([this, pathToFile = entry.path()]() {
                if (pathToFile.extension() != ".toml") {
                    return;
                }
                const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
                if (pGameInstance == nullptr) [[unlikely]] {
                    Error::showErrorAndThrowException("expected editor game instance to be valid");
                }
                pGameInstance->openNodeTreeAsGameWorld(pathToFile);
            });
        }
        {
            const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
            pText->setTextHeight(EditorTheme::getTextHeight());
            pText->setText(utf::as_u16(sEntryName));
        }
    }
}

void ContentBrowser::onDirectoryRightClick(const std::filesystem::path& pathToDirectory) {
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
            {u"Import texture", [this, pathToDirectory]() {
                 getWorldRootNodeWhileSpawned()->addChildNode(std::make_unique<FileDialogMenu>(
                     pathToDirectory, [this, pathToDirectory](const std::filesystem::path& selectedPath) {
                         auto optionalError =
                             TextureManager::importTextureFromFile(selectedPath, pathToDirectory);
                         if (optionalError.has_value()) [[unlikely]] {
                             Logger::get().error(std::format(
                                 "failed to import texture, error: {}", optionalError->getInitialMessage()));
                         } else {
                             Logger::get().info(std::format(
                                 "texture \"{}\" was successfully imported", selectedPath.stem().string()));
                             rebuildFileTree();
                         }
                     }));
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
                         Logger::get().error(
                             std::format("unable to create directory, error: {}", exception.what()));
                     }
                     rebuildFileTree();
                 });
             }});
        vOptions.push_back({u"Delete directory", [this, pathToDirectory]() {
                                openedDirectoryPaths.erase(pathToDirectory);
                                std::filesystem::remove_all(pathToDirectory);
                                rebuildFileTree();
                            }});
    }
    dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned())->openContextMenu(vOptions);
}
