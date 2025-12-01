#include "node/LogViewNode.h"

// Custom.
#include "game/node/ui/LayoutUiNode.h"
#include "game/node/ui/TextUiNode.h"
#include "EditorTheme.h"

// External.
#include "utf/utf.hpp"

LogViewNode::LogViewNode() : LogViewNode("Log View Node") {}
LogViewNode::LogViewNode(const std::string& sNodeName) : RectUiNode(sNodeName) {
    setPadding(EditorTheme::getPadding());
    setColor(EditorTheme::getEditorBackgroundColor());

    pLayout = addChildNode(std::make_unique<LayoutUiNode>("Log View Layout"));
    pLayout->setChildNodeExpandRule(ChildNodeExpandRule::EXPAND_ALONG_SECONDARY_AXIS);
    pLayout->setIsScrollBarEnabled(true);
    pLayout->setAutoScrollToBottom(true);

    pLoggerCallback =
        Log::setCallback([this](LogMessageCategory category, const std::string& sMessage) {
            const auto pTextNode = pLayout->addChildNode(std::make_unique<TextUiNode>("Log View Message"));
            pTextNode->setTextHeight(EditorTheme::getSmallTextHeight() * 0.95f);
            pTextNode->setText(utf::as_u16(sMessage));

            if (category == LogMessageCategory::ERROR) {
                pTextNode->setTextColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            } else if (category == LogMessageCategory::WARNING) {
                pTextNode->setTextColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
            }
        });
}

void LogViewNode::onDespawning() {
    RectUiNode::onDespawning();

    pLoggerCallback = nullptr;
}
