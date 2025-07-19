#include "node/menu/FileDialogMenu.h"

// Custom.
#include "EditorTheme.h"
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/ButtonUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
#include "EditorGameInstance.h"

// External.
#include "utf/utf.hpp"

FileDialogMenu::FileDialogMenu(
    const std::filesystem::path& pathToDirectory,
    const std::function<void(const std::filesystem::path& path)>& onSelected)
    : RectUiNode("File Dialog"), onSelected(onSelected) {
    setPosition(glm::vec2(0.0F, 0.0F));
    setSize(glm::vec2(1.0F, 1.0F));
    setColor(glm::vec4(0.0F, 0.0F, 0.0F, 0.5F));
    setUiLayer(UiLayer::LAYER2);
    setModal();

    const auto pMenuBackground = addChildNode(std::make_unique<RectUiNode>());
    pMenuBackground->setPosition(glm::vec2(0.25F, 0.25F));
    pMenuBackground->setSize(glm::vec2(0.5F, 0.5F));
    pMenuBackground->setColor(EditorTheme::getContainerBackgroundColor());
    pMenuBackground->setPadding(EditorTheme::getPadding());
    {
        const auto pVerticalLayout = pMenuBackground->addChildNode(std::make_unique<LayoutUiNode>());
        pVerticalLayout->setChildNodeSpacing(EditorTheme::getSpacing() * 2.0F);
        pVerticalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
        {
            const auto pHorizontalLayout = pVerticalLayout->addChildNode(std::make_unique<LayoutUiNode>());
            pHorizontalLayout->setIsHorizontal(true);
            pHorizontalLayout->setChildNodeSpacing(EditorTheme::getSpacing() * 2.0F);
            pHorizontalLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_BOTH_AXIS);
            {
                const auto pButton = pHorizontalLayout->addChildNode(std::make_unique<ButtonUiNode>());
                pButton->setPadding(EditorTheme::getPadding() * 2.0F);
                pButton->setExpandPortionInLayout(2);
                pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
                pButton->setColor(EditorTheme::getButtonColor());
                pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
                pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
                pButton->setOnClicked([this]() {
                    if (!pathToCurrentDirectory.has_parent_path()) {
                        return;
                    }
                    showDirectory(pathToCurrentDirectory.parent_path());
                });
                {
                    const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
                    pText->setTextHeight(EditorTheme::getTextHeight());
                    pText->setText(u"go up");
                }

                const auto pCurrentPathBackground =
                    pHorizontalLayout->addChildNode(std::make_unique<RectUiNode>());
                pCurrentPathBackground->setExpandPortionInLayout(18);
                pCurrentPathBackground->setColor(EditorTheme::getButtonColor());
                pCurrentPathBackground->setPadding(EditorTheme::getPadding());
                pCurrentPathBackground->setSize(
                    glm::vec2(pCurrentPathBackground->getSize().x, EditorTheme::getButtonSizeY()));
                {
                    pCurrentPathText = pCurrentPathBackground->addChildNode(std::make_unique<TextUiNode>());
                    pCurrentPathText->setTextHeight(EditorTheme::getTextHeight());
                }

                const auto pCloseButton = pHorizontalLayout->addChildNode(std::make_unique<ButtonUiNode>());
                pCloseButton->setPadding(EditorTheme::getPadding() * 2.0F);
                pCloseButton->setExpandPortionInLayout(2);
                pCloseButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
                pCloseButton->setColor(EditorTheme::getButtonColor());
                pCloseButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
                pCloseButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
                pCloseButton->setOnClicked([this]() { unsafeDetachFromParentAndDespawn(true); });
                {
                    const auto pText = pCloseButton->addChildNode(std::make_unique<TextUiNode>());
                    pText->setTextHeight(EditorTheme::getTextHeight());
                    pText->setText(u"cancel");
                }
            }

            const auto pFilesystemBackground = pVerticalLayout->addChildNode(std::make_unique<RectUiNode>());
            pFilesystemBackground->setColor(EditorTheme::getContainerBackgroundColor());
            pFilesystemBackground->setExpandPortionInLayout(16);
            pFilesystemBackground->setPadding(EditorTheme::getPadding());
            {
                pFilesystemLayout = pFilesystemBackground->addChildNode(std::make_unique<LayoutUiNode>());
                pFilesystemLayout->setIsScrollBarEnabled(true);
                pFilesystemLayout->setChildNodeSpacing(EditorTheme::getSpacing());
                pFilesystemLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
            }
        }

        showDirectory(pathToDirectory);
    }
}

void FileDialogMenu::onChildNodesSpawned() {
    RectUiNode::onChildNodesSpawned();

    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance to be valid");
    }

    pGameInstance->setEnableViewportCamera(false);
}

void FileDialogMenu::onDespawning() {
    RectUiNode::onDespawning();

    const auto pGameInstance = dynamic_cast<EditorGameInstance*>(getGameInstanceWhileSpawned());
    if (pGameInstance == nullptr) [[unlikely]] {
        Error::showErrorAndThrowException("expected editor game instance to be valid");
    }

    pGameInstance->setEnableViewportCamera(true);
}

void FileDialogMenu::showDirectory(std::filesystem::path pathToDirectory) {
    {
        const auto mtxChildNodes = pFilesystemLayout->getChildNodes();
        std::scoped_lock guard(*mtxChildNodes.first);
        for (const auto& pNode : mtxChildNodes.second) {
            pNode->unsafeDetachFromParentAndDespawn(true);
        }
    }

    pathToCurrentDirectory = pathToDirectory;

    pCurrentPathText->setText(utf::as_u16(pathToDirectory.string()));

    if (std::filesystem::is_empty(pathToDirectory)) {
        const auto pText = pFilesystemLayout->addChildNode(std::make_unique<TextUiNode>());
        pText->setTextHeight(EditorTheme::getTextHeight());
        pText->setText(u"Directory is empty.");
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(pathToDirectory)) {
        std::string sEntryName;

        const auto pButton = pFilesystemLayout->addChildNode(std::make_unique<ButtonUiNode>());
        pButton->setPadding(EditorTheme::getPadding());
        pButton->setSize(glm::vec2(pButton->getSize().x, EditorTheme::getButtonSizeY()));
        pButton->setColor(EditorTheme::getButtonColor());
        pButton->setColorWhileHovered(EditorTheme::getButtonHoverColor());
        pButton->setColorWhilePressed(EditorTheme::getButtonPressedColor());
        if (std::filesystem::is_directory(entry)) {
            sEntryName = "[/] " + entry.path().filename().string();
            pButton->setOnClicked([this, pathToDir = entry.path()]() { showDirectory(pathToDir); });
        } else {
            sEntryName = entry.path().filename().string();
            pButton->setOnClicked([this, pathToFile = entry.path()]() {
                onSelected(pathToFile);
                unsafeDetachFromParentAndDespawn(true);
            });
        }
        {
            const auto pText = pButton->addChildNode(std::make_unique<TextUiNode>());
            pText->setTextHeight(EditorTheme::getTextHeight());
            pText->setText(utf::as_u16(sEntryName));
        }
    }
}
