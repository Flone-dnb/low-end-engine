#include "render/UiManager.h"

// Standard.
#include <format>
#include <ranges>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "render/ShaderManager.h"
#include "render/Renderer.h"
#include "misc/Error.h"
#include "render/font/FontManager.h"
#include "render/wrapper/ShaderProgram.h"
#include "render/wrapper/Texture.h"
#include "game/Window.h"
#include "render/GpuResourceManager.h"

// External.
#include "glad/glad.h"

#define ADD_NODE_TO_RENDERING(nodeType)                                                                      \
    /** Find an array of nodes to add the node to according to the node's depth. */                          \
    std::unordered_set<nodeType*>* pNodeArray = nullptr;                                                     \
    const auto iNodeDepth = pNode->getNodeDepthWhileSpawned();                                               \
    for (size_t i = 0; i < vNodesByDepth.size(); i++) {                                                      \
        if (vNodesByDepth[i].first == iNodeDepth) {                                                          \
            pNodeArray = &vNodesByDepth[i].second;                                                           \
            break;                                                                                           \
        } else if (vNodesByDepth[i].first > iNodeDepth) { /** NOLINT: else after break */                    \
            break;                                                                                           \
        }                                                                                                    \
    }                                                                                                        \
                                                                                                             \
    if (pNodeArray == nullptr) {                                                                             \
        vNodesByDepth.push_back({iNodeDepth, {pNode}});                                                      \
        std::sort(                                                                                           \
            vNodesByDepth.begin(),                                                                           \
            vNodesByDepth.end(),                                                                             \
            [](const std::pair<size_t, std::unordered_set<nodeType*>>& left,                                 \
               const std::pair<size_t, std::unordered_set<nodeType*>>& right) -> bool {                      \
                return left.first < right.first;                                                             \
            });                                                                                              \
    } else {                                                                                                 \
        const auto [it, isAdded] = pNodeArray->insert(pNode);                                                \
        if (!isAdded) [[unlikely]] {                                                                         \
            Error::showErrorAndThrowException(                                                               \
                std::format("node \"{}\" is already added", pNode->getNodeName()));                          \
        }                                                                                                    \
    }

#define REMOVE_NODE_FROM_RENDERING(nodeType)                                                                 \
    /** Find an array of nodes to remove the node from node's depth. */                                      \
    std::optional<size_t> optionalArrayIndex;                                                                \
    const auto iNodeDepth = pNode->getNodeDepthWhileSpawned();                                               \
    for (size_t i = 0; i < vNodesByDepth.size(); i++) {                                                      \
        if (vNodesByDepth[i].first == iNodeDepth) {                                                          \
            optionalArrayIndex = i;                                                                          \
            break;                                                                                           \
        } else if (vNodesByDepth[i].first > iNodeDepth) { /** NOLINT: else after break */                    \
            break;                                                                                           \
        }                                                                                                    \
    }                                                                                                        \
    if (!optionalArrayIndex.has_value()) [[unlikely]] {                                                      \
        Error::showErrorAndThrowException(                                                                   \
            std::format(                                                                                     \
                "unable to find the node \"{}\" with depth {} to remove from rendering",                     \
                pNode->getNodeName(),                                                                        \
                iNodeDepth));                                                                                \
    }                                                                                                        \
    const auto iArrayIndex = *optionalArrayIndex;                                                            \
                                                                                                             \
    vNodesByDepth[iArrayIndex].second.erase(pNode);                                                          \
    if (vNodesByDepth[iArrayIndex].second.empty()) {                                                         \
        vNodesByDepth.erase(vNodesByDepth.begin() + iArrayIndex);                                            \
    }

void UiManager::onNodeSpawning(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(TextUiNode);

    if (mtxData.second.pTextShaderProgram == nullptr) {
        // Load shader.
        mtxData.second.pTextShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl",
            "engine/shaders/ui/TextNode.frag.glsl",
            ShaderProgramUsage::OTHER);
    }
}

void UiManager::onNodeSpawning(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(RectUiNode);

    if (mtxData.second.pRectShaderProgram == nullptr) {
        // Load shader.
        mtxData.second.pRectShaderProgram = pRenderer->getShaderManager().getShaderProgram(
            "engine/shaders/ui/UiScreenQuad.vert.glsl",
            "engine/shaders/ui/RectUiNode.frag.glsl",
            ShaderProgramUsage::OTHER);
    }
}

void UiManager::onSpawnedNodeChangedVisibility(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (pNode->isVisible()) {
        ADD_NODE_TO_RENDERING(TextUiNode);
    } else {
        REMOVE_NODE_FROM_RENDERING(TextUiNode);
    }
}

void UiManager::onSpawnedNodeChangedVisibility(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (pNode->isVisible()) {
        ADD_NODE_TO_RENDERING(RectUiNode);
    } else {
        REMOVE_NODE_FROM_RENDERING(RectUiNode);
    }
}

void UiManager::onNodeDespawning(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (!pNode->isVisible()) {
        return;
    }

    REMOVE_NODE_FROM_RENDERING(TextUiNode);

    if (vNodesByDepth.empty()) {
        mtxData.second.pTextShaderProgram = nullptr;
    }
}

void UiManager::onNodeDespawning(RectUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;

    if (!pNode->isVisible()) {
        return;
    }

    REMOVE_NODE_FROM_RENDERING(RectUiNode);

    if (vNodesByDepth.empty()) {
        mtxData.second.pRectShaderProgram = nullptr;
    }
}

void UiManager::onNodeChangedDepth(UiNode* pTargetNode) {
    std::scoped_lock guard(mtxData.first);

    if (!pTargetNode->isVisible()) {
        return;
    }

    if (auto pNode = dynamic_cast<TextUiNode*>(pTargetNode)) {
        auto& vNodesByDepth =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;
        {
            REMOVE_NODE_FROM_RENDERING(TextUiNode);
        }
        {
            ADD_NODE_TO_RENDERING(TextUiNode);
        }
    } else if (auto pNode = dynamic_cast<RectUiNode*>(pTargetNode)) {
        auto& vNodesByDepth =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vRectNodes;
        {
            REMOVE_NODE_FROM_RENDERING(RectUiNode);
        }
        {
            ADD_NODE_TO_RENDERING(RectUiNode);
        }
    } else [[unlikely]] {
        Error::showErrorAndThrowException("unhandled case");
    }

#if defined(WIN32) && defined(DEBUG)
    static_assert(
        sizeof(Data::SpawnedVisibleUiNodes) == 136, "add new variables here"); // NOLINT: current size
#endif
}

void UiManager::onSpawnedUiNodeInputStateChange(UiNode* pNode, bool bEnabledInput) {
    std::scoped_lock guard(mtxData.first);
    auto& nodes =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].receivingInputUiNodes;

    if (bEnabledInput) {
        if (!nodes.insert(pNode).second) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format(
                    "spawned node \"{}\" enabled input but it already exists in UI manager's array of nodes "
                    "that receive input",
                    pNode->getNodeName()));
        }
    } else {
        const auto it = nodes.find(pNode);
        if (it == nodes.end()) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format(
                    "unable to find spawned node \"{}\" to remove from the array of nodes that receive input",
                    pNode->getNodeName()));
        }
        nodes.erase(it);

        // Remove from rendered last frame in order to avoid triggering input on node after it was despawned.
        auto& vInputNodesRenderedLastFrame =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())]
                .receivingInputUiNodesRenderedLastFrame;
        for (size_t i = 0; i < vInputNodesRenderedLastFrame.size(); i++) {
            if (vInputNodesRenderedLastFrame[i] == pNode) {
                vInputNodesRenderedLastFrame.erase(vInputNodesRenderedLastFrame.begin() + i);
                break;
            }
        }

        if (mtxData.second.pFocusedNode == pNode) {
            mtxData.second.pFocusedNode = nullptr;
        }
        if (mtxData.second.pHoveredNodeLastFrame == pNode) {
            mtxData.second.pHoveredNodeLastFrame = nullptr;
        }
    }
}

void UiManager::onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode == nullptr) {
        return;
    }

    mtxData.second.pFocusedNode->onKeyboardInputOnUiNode(key, modifiers, bIsPressedDown);
}

void UiManager::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxData.first);

    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    const auto [iCursorX, iCursorY] = pRenderer->getWindow()->getCursorPosition();

    const glm::vec2 cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWidth),
        1.0F - static_cast<float>(iCursorY) / static_cast<float>(iHeight)); // flip Y

    // Check rendered input nodes in reverse order (from front layer to back layer).
    for (const auto& layerNodes : std::views::reverse(mtxData.second.vSpawnedVisibleNodes)) {
        bool bFoundNode = false;

        for (auto& pNode : layerNodes.receivingInputUiNodesRenderedLastFrame) {
            const auto leftBottomPos = pNode->getPosition();
            const auto size = pNode->getSize();

            if (leftBottomPos.y > cursorPos.y) {
                continue;
            }
            if (leftBottomPos.x > cursorPos.x) {
                continue;
            }
            if (leftBottomPos.y + size.y < cursorPos.y) {
                continue;
            }
            if (leftBottomPos.x + size.x < cursorPos.x) {
                continue;
            }

            bFoundNode = true;
            pNode->onMouseClickOnUiNode(button, modifiers, bIsPressedDown);
            break;
        }

        if (bFoundNode) {
            break;
        }
    }
}

void UiManager::onMouseMove(int iXOffset, int iYOffset) {
    std::scoped_lock guard(mtxData.first);

    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    const auto [iCursorX, iCursorY] = pRenderer->getWindow()->getCursorPosition();

    const glm::vec2 cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWidth),
        1.0F - static_cast<float>(iCursorY) / static_cast<float>(iHeight)); // flip Y

    // Check rendered input nodes in reverse order (from front layer to back layer).
    bool bFoundNode = false;
    for (const auto& layerNodes : std::views::reverse(mtxData.second.vSpawnedVisibleNodes)) {
        for (auto& pNode : layerNodes.receivingInputUiNodesRenderedLastFrame) {
            const auto leftBottomPos = pNode->getPosition();
            const auto size = pNode->getSize();

            if (leftBottomPos.y > cursorPos.y) {
                continue;
            }
            if (leftBottomPos.x > cursorPos.x) {
                continue;
            }
            if (leftBottomPos.y + size.y < cursorPos.y) {
                continue;
            }
            if (leftBottomPos.x + size.x < cursorPos.x) {
                continue;
            }

            bFoundNode = true;
            pNode->bIsHoveredCurrFrame = true;
            mtxData.second.pHoveredNodeLastFrame = pNode;

            if (!pNode->bIsHoveredPrevFrame) {
                pNode->onMouseEntered();
            }

            // mouse move is called by game manager not us
            break;
        }

        if (bFoundNode) {
            break;
        }
    }

    if (!bFoundNode) {
        mtxData.second.pHoveredNodeLastFrame = nullptr;
    }

    mtxData.second.bWasHoveredNodeCheckedThisFrame = true;
}

void UiManager::onMouseScrollMove(int iOffset) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pHoveredNodeLastFrame == nullptr) {
        return;
    }

    mtxData.second.pHoveredNodeLastFrame->onMouseScrollMoveWhileHovered(iOffset);
}

UiManager::UiManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    uiProjMatrix = glm::ortho(0.0F, static_cast<float>(iWidth), 0.0F, static_cast<float>(iHeight));

    mtxData.second.pScreenQuadGeometry = GpuResourceManager::createQuad(true);
}

void UiManager::drawUi(unsigned int iDrawFramebufferId) {
    PROFILE_FUNC;

    glBindFramebuffer(GL_FRAMEBUFFER, iDrawFramebufferId);

    std::scoped_lock guard(mtxData.first);

    if (pRenderer->getWindow()->isCursorVisible() && !mtxData.second.bWasHoveredNodeCheckedThisFrame) {
        onMouseMove(0, 0);
    }

    for (auto& nodes : mtxData.second.vSpawnedVisibleNodes) {
        nodes.receivingInputUiNodesRenderedLastFrame.clear(); // clear but don't shrink
    }

    glDisable(GL_DEPTH_TEST);
    for (size_t i = 0; i < mtxData.second.vSpawnedVisibleNodes.size(); i++) {
        drawRectNodes(i);
        drawTextNodes(i);
    }
    glEnable(GL_DEPTH_TEST);

    mtxData.second.bWasHoveredNodeCheckedThisFrame = false;
}

void UiManager::drawRectNodes(size_t iLayer) {
    std::scoped_lock guard(mtxData.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vRectNodes;
    if (vNodesByDepth.empty()) {
        return;
    }
    auto& vInputNodesRendered =
        mtxData.second.vSpawnedVisibleNodes[iLayer].receivingInputUiNodesRenderedLastFrame;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

        // Set shader program.
        auto& pShaderProgram = mtxData.second.pRectShaderProgram;
        if (pShaderProgram == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the shader to be loaded at this point");
        }
        glUseProgram(pShaderProgram->getShaderProgramId());

        glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

        pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pRectNode : nodes) {
                // Update input-related things.
                if (pRectNode->isReceivingInputUnsafe()) { // safe - node won't despawn/change state here
                                                           // (it will wait on our mutex)
                    vInputNodesRendered.push_back(pRectNode);
                }
                if (!pRectNode->bIsHoveredCurrFrame && pRectNode->bIsHoveredPrevFrame) {
                    pRectNode->onMouseLeft();
                }
                pRectNode->bIsHoveredPrevFrame = pRectNode->bIsHoveredCurrFrame;
                pRectNode->bIsHoveredCurrFrame = false;

                auto pos = pRectNode->getPosition();
                auto size = pRectNode->getSize();

                // Set shader parameters.
                pShaderProgram->setVector4ToShader("color", pRectNode->getColor());
                if (pRectNode->pTexture != nullptr) {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, pRectNode->pTexture->getTextureId());
                } else {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", false);
                }

                pos = glm::vec2(pos.x * iWindowWidth, pos.y * iWindowHeight);
                size = glm::vec2(size.x * iWindowWidth, size.y * iWindowHeight);

                // Update VBO.
                const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                    glm::vec4(pos.x, pos.y + size.y, 0.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y, 1.0F, 1.0F),
                    glm::vec4(pos.x, pos.y, 0.0F, 1.0F),

                    glm::vec4(pos.x, pos.y + size.y, 0.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y + size.y, 1.0F, 0.0F),
                    glm::vec4(pos.x + size.x, pos.y, 1.0F, 1.0F)};

                // Copy new vertex data to VBO.
                glBindBuffer(
                    GL_ARRAY_BUFFER, mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Render quad.
                glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawTextNodes(size_t iLayer) {
    auto& mtxLoadedGlyphs = pRenderer->getFontManager().getLoadedGlyphs();
    std::scoped_lock guard(mtxData.first, mtxLoadedGlyphs.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vTextNodes;
    if (vNodesByDepth.empty()) {
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

        // Set shader program.
        auto& pShaderProgram = mtxData.second.pTextShaderProgram;
        if (pShaderProgram == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected the shader to be loaded at this point");
        }
        glUseProgram(pShaderProgram->getShaderProgramId());

        glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

        pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);
        glActiveTexture(GL_TEXTURE0); // glyph's bitmap

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pTextNode : nodes) {
                // Prepare some variables.
                const auto textRelativePos = pTextNode->getPosition();

                float screenX = textRelativePos.x * iWindowWidth;
                float screenY = textRelativePos.y * iWindowHeight;
                const auto scale = pTextNode->getSize().y / FontManager::getFontHeightToLoad();

                const float textHeightInPixels = iWindowHeight * FontManager::getFontHeightToLoad() * scale;
                const float lineSpacingInPixels = pTextNode->getTextLineSpacing() * textHeightInPixels;

                // Set color.
                pShaderProgram->setVector4ToShader("textColor", pTextNode->getTextColor());

                // Render each character.
                for (const auto& character : pTextNode->getText()) {
                    // Handle new line.
                    if (character == '\n') {
                        screenY -= textHeightInPixels + lineSpacingInPixels;
                        screenX = textRelativePos.x * iWindowWidth;
                        continue;
                    }

                    // Get glyph.
                    auto charIt = mtxLoadedGlyphs.second.find(character);
                    if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                        // No glyph found for this character, use ? instead.
                        // DON'T log a warning - you will slow everything down due to log flushing.
                        charIt = mtxLoadedGlyphs.second.find('?');
                        if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                            Error::showErrorAndThrowException(
                                std::format(
                                    "unable to find loaded glyph to render the character \"{}\" and also "
                                    "can't find a glyph for `?` to render it instead",
                                    character));
                        }
                    }
                    const auto& glyph = charIt->second;

                    float xpos = screenX + glyph.bearing.x * scale;
                    float ypos = screenY - (glyph.size.y - glyph.bearing.y) * scale;

                    float width = static_cast<float>(glyph.size.x) * scale;
                    float height = static_cast<float>(glyph.size.y) * scale;

                    // Update VBO.
                    const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F),
                        glm::vec4(xpos, ypos, 0.0F, 1.0F),

                        glm::vec4(xpos, ypos + height, 0.0F, 0.0F),
                        glm::vec4(xpos + width, ypos + height, 1.0F, 0.0F),
                        glm::vec4(xpos + width, ypos, 1.0F, 1.0F)};

                    glBindTexture(GL_TEXTURE_2D, glyph.pTexture->getTextureId());

                    // Copy new vertex data to VBO.
                    glBindBuffer(
                        GL_ARRAY_BUFFER,
                        mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    // Render quad.
                    glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);

                    // Advance current pos to the next glyph (note that advance is number of 1/64 pixels).
                    screenX +=
                        (glyph.advance >> 6) * // NOLINT: bitshift by 6 to get value in pixels (2^6 = 64)
                        scale;
                }
            }
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_BLEND);
}

UiManager::~UiManager() {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "UI manager is being destroyed but focused node pointer is still not `nullptr`");
    }
    if (mtxData.second.pHoveredNodeLastFrame != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "UI manager is being destroyed but hovered node pointer is still not `nullptr`");
    }

    // Make sure all nodes removed.
    size_t iNodeCount = 0;
    for (const auto& nodes : mtxData.second.vSpawnedVisibleNodes) {
        iNodeCount += nodes.getTotalNodeCount();
    }
    if (iNodeCount != 0) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "UI manager is being destroyed but there are still {} spawned and visible nodes",
                iNodeCount));
    }
}
