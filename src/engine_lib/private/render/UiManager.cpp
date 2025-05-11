#include "render/UiManager.h"

// Standard.
#include <format>
#include <ranges>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "render/ShaderManager.h"
#include "render/Renderer.h"
#include "misc/Error.h"
#include "render/FontManager.h"
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

    // don't unload rect shader program because it's also used for drawing cursors
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

void collectInputReceivingChildNodes(UiNode* pParent, std::unordered_set<UiNode*>& inputReceivingNodes) {
    if (pParent->isReceivingInput()) {
        inputReceivingNodes.insert(pParent);
    }

    const auto mtxChildNodes = pParent->getChildNodes();
    std::scoped_lock guard(*mtxChildNodes.first);
    for (const auto& pChildNode : mtxChildNodes.second) {
        const auto pUiNode = dynamic_cast<UiNode*>(pChildNode);
        if (pUiNode == nullptr) [[unlikely]] {
            Error::showErrorAndThrowException("expected a UI node");
        }

        collectInputReceivingChildNodes(pUiNode, inputReceivingNodes);
    }
}

void UiManager::setModalNode(UiNode* pNewModalNode) {
    std::scoped_lock guard(mtxData.first);

    mtxData.second.modalInputReceivingNodes.clear();

    if (pNewModalNode == nullptr) {
        return;
    }

    // Collect all child nodes that receive input.
    std::unordered_set<UiNode*> inputReceivingNodes;
    collectInputReceivingChildNodes(pNewModalNode, inputReceivingNodes);

    if (inputReceivingNodes.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "unable to make a modal node because the node or its child nodes don't receive input");
    }

    // Make sure there are all spawned and visible (i.e. stored in our arrays so that we will automatically
    // clean modalitty on them in case they became invisible or despawn or etc.).
    // Also make deepest node a focused one.
    UiNode* pDeepestNode = nullptr;
    size_t iNodeDepth = 0;
    for (const auto& pNode : inputReceivingNodes) {
        bool bFound = false;

        if (pNode->iNodeDepth > iNodeDepth) {
            iNodeDepth = pNode->iNodeDepth;
            pDeepestNode = pNode;
        }

        for (const auto& layerNodes : mtxData.second.vSpawnedVisibleNodes) {
            const auto it = layerNodes.receivingInputUiNodes.find(pNode);
            if (it != layerNodes.receivingInputUiNodes.end()) {
                bFound = true;
                break;
            }
        }

        if (!bFound) [[unlikely]] {
            Error::showErrorAndThrowException(
                std::format(
                    "unable to find node \"{}\" to be spawned, visible and receiving input to make modal",
                    pNode->getNodeName()));
        }
    }

    if (pDeepestNode == nullptr) {
        if (inputReceivingNodes.size() == 1) {
            pDeepestNode = *inputReceivingNodes.begin();
        } else {
            Error::showErrorAndThrowException("unexpected case");
        }
    }

    mtxData.second.modalInputReceivingNodes = std::move(inputReceivingNodes);
    changeFocusedNode(pDeepestNode);
}

void UiManager::setFocusedNode(UiNode* pFocusedNode) {
    std::scoped_lock guard(mtxData.first);

    // Find in our arrays so that we will automatically clean focus state when becomes invisible or despawns.
    bool bFound = false;

    for (const auto& layerNodes : mtxData.second.vSpawnedVisibleNodes) {
        const auto it = layerNodes.receivingInputUiNodes.find(pFocusedNode);
        if (it != layerNodes.receivingInputUiNodes.end()) {
            bFound = true;
            break;
        }
    }

    if (!bFound) [[unlikely]] {
        Error::showErrorAndThrowException(
            std::format(
                "unable to find node \"{}\" to be spawned, visible and receiving input to make focused",
                pFocusedNode->getNodeName()));
    }

    changeFocusedNode(pFocusedNode);
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
            changeFocusedNode(nullptr);
        }
        if (mtxData.second.pHoveredNodeLastFrame == pNode) {
            mtxData.second.pHoveredNodeLastFrame = nullptr;
        }

        if (!mtxData.second.modalInputReceivingNodes.empty()) {
            mtxData.second.modalInputReceivingNodes.erase(pNode);
        }
    }
}

void UiManager::onKeyboardInput(KeyboardButton key, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode == nullptr) {
        return;
    }

    mtxData.second.pFocusedNode->onKeyboardInputWhileFocused(key, modifiers, bIsPressedDown);
}

void UiManager::onKeyboardInputTextCharacter(const std::string& sTextCharacter) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode == nullptr) {
        return;
    }

    mtxData.second.pFocusedNode->onKeyboardInputTextCharacterWhileFocused(sTextCharacter);
}

void UiManager::onMouseInput(MouseButton button, KeyboardModifiers modifiers, bool bIsPressedDown) {
    std::scoped_lock guard(mtxData.first);

    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    const auto [iCursorX, iCursorY] = pRenderer->getWindow()->getCursorPosition();

    const glm::vec2 cursorPos = glm::vec2(
        static_cast<float>(iCursorX) / static_cast<float>(iWidth),
        1.0F - static_cast<float>(iCursorY) / static_cast<float>(iHeight)); // flip Y

    if (!mtxData.second.modalInputReceivingNodes.empty()) {
        for (auto& pNode : mtxData.second.modalInputReceivingNodes) {
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

            changeFocusedNode(pNode);
            pNode->onMouseClickOnUiNode(button, modifiers, bIsPressedDown);
            break;
        }

        return;
    }

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
            changeFocusedNode(pNode);
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

    bool bFoundNode = false;

    if (!mtxData.second.modalInputReceivingNodes.empty()) {
        for (auto& pNode : mtxData.second.modalInputReceivingNodes) {
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

            pNode->bIsHoveredCurrFrame = true;
            mtxData.second.pHoveredNodeLastFrame = pNode;

            if (!pNode->bIsHoveredPrevFrame) {
                pNode->onMouseEntered();
            }

            // mouse move is called by game manager not us
            break;
        }
    } else {
        // Check rendered input nodes in reverse order (from front layer to back layer).
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

bool UiManager::hasModalUiNodeTree() {
    std::scoped_lock guard(mtxData.first);

    return !mtxData.second.modalInputReceivingNodes.empty();
}

UiManager::UiManager(Renderer* pRenderer) : pRenderer(pRenderer) {
    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    uiProjMatrix = glm::ortho(0.0F, static_cast<float>(iWidth), 0.0F, static_cast<float>(iHeight));

    mtxData.second.pScreenQuadGeometry = GpuResourceManager::createQuad(true);

    // Load shader.
    mtxData.second.pRectAndCursorShaderProgram = pRenderer->getShaderManager().getShaderProgram(
        "engine/shaders/ui/UiScreenQuad.vert.glsl",
        "engine/shaders/ui/RectUiNode.frag.glsl",
        ShaderProgramUsage::OTHER);
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
    PROFILE_FUNC;

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
        auto& pShaderProgram = mtxData.second.pRectAndCursorShaderProgram;
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

void UiManager::drawTextNodes(size_t iLayer) { // NOLINT
    PROFILE_FUNC;

    auto& mtxLoadedGlyphs = pRenderer->getFontManager().getLoadedGlyphs();
    std::scoped_lock guard(mtxData.first, mtxLoadedGlyphs.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vTextNodes;
    if (vNodesByDepth.empty()) {
        return;
    }
    auto& vInputNodesRendered =
        mtxData.second.vSpawnedVisibleNodes[iLayer].receivingInputUiNodesRenderedLastFrame;

    // Prepare a placeholder glyph for unknown glyphs.
    const auto placeHolderGlythIt = mtxLoadedGlyphs.second.find('?');
    if (placeHolderGlythIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
        Error::showErrorAndThrowException("can't find a glyph for `?`");
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

        // Prepare info to later draw cursors for text edit UI nodes.
        struct CursorDrawInfo {
            glm::vec2 screenPos = glm::vec2(0.0F, 0.0F);
            float height = 0.0F;
        };
        std::vector<CursorDrawInfo> vCursorScreenPosToDraw;

        // Prepare info to later draw text selection.
        struct TextSelectionDrawInfo {
            std::vector<std::pair<glm::vec2, glm::vec2>> vLineStartEndScreenPos;
            float textHeightInPixels;
            glm::vec4 color;
        };
        std::vector<TextSelectionDrawInfo> vTextSelectionToDraw;

        // Prepare info to later draw scroll bars.
        struct ScrollBarDrawInfo {
            glm::vec2 posInPixels = glm::vec2(0.0F, 0.0F);
            float widthInPixels = 0.0F;
            float heightInPixels = 0.0F;
            float verticalPos = 0.0F;  // in [0.0; 1.0]
            float verticalSize = 0.0F; // in [0.0; 1.0]
            glm::vec4 color;
        };
        std::vector<ScrollBarDrawInfo> vScrollBarToDraw;

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pTextNode : nodes) {
                // Check cursor and selection.
                std::optional<size_t> optionalCursorOffset;
                std::optional<std::pair<size_t, size_t>> optionalSelection;
                glm::vec4 selectionColor;
                if (auto pTextEdit = dynamic_cast<TextEditUiNode*>(pTextNode)) {
                    if (pTextEdit->isReceivingInputUnsafe()) { // safe - node won't despawn/change state here
                                                               // (it will wait on our mutex)
                        vInputNodesRendered.push_back(pTextNode);
                    }
                    optionalCursorOffset = pTextEdit->optionalCursorOffset;
                    optionalSelection = pTextEdit->optionalSelection;
                    selectionColor = pTextEdit->getTextSelectionColor();
                }
                bool bSelectionStartPosFound = false;
                std::vector<std::pair<glm::vec2, glm::vec2>> vSelectionLinesToDraw;

                // Prepare some variables for rendering.
                const auto sText = pTextNode->getText();
                const auto leftBottomTextPos = pTextNode->getPosition();
                const auto screenMaxXForWordWrap =
                    (leftBottomTextPos.x + pTextNode->getSize().x) * iWindowWidth;

                float screenX = leftBottomTextPos.x * iWindowWidth;
                float screenY = (1.0F - leftBottomTextPos.y) * iWindowHeight;
                const auto screenYEnd = screenY - pTextNode->getSize().y * iWindowHeight;
                const auto scale = pTextNode->getTextHeight() / FontManager::getFontHeightToLoad();

                const float textHeightInPixels = iWindowHeight * FontManager::getFontHeightToLoad() * scale;
                const float lineSpacingInPixels = pTextNode->getTextLineSpacing() * textHeightInPixels;

                // Check scroll bar.
                size_t iLinesToSkip = 0;
                if (pTextNode->getIsScrollBarEnabled()) {
                    iLinesToSkip = pTextNode->getCurrentScrollOffset();
                }

                // Set color.
                pShaderProgram->setVector4ToShader("textColor", pTextNode->getTextColor());

                // Switch to the first row of text.
                screenY -= textHeightInPixels;

                // Render each character.
                size_t iLineIndex = 0;
                size_t iRenderedCharCount = 0;
                size_t iCharIndex = 0;
                bool bReachedEndOfUiNode = false;
                for (; iCharIndex < sText.size(); iCharIndex++) {
                    const char& character = sText[iCharIndex];

                    // Prepare a handy lambda.
                    const auto switchToNewLine = [&]() {
                        // Check cursor.
                        if (optionalCursorOffset.has_value() && *optionalCursorOffset == iCharIndex) {
                            vCursorScreenPosToDraw.push_back(
                                CursorDrawInfo{
                                    .screenPos = glm::vec2(screenX, screenY),
                                    .height = FontManager::getFontHeightToLoad() * scale});
                        }

                        // Check selection.
                        if (optionalSelection.has_value() && bSelectionStartPosFound) {
                            vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);

                            if (optionalSelection->second == iCharIndex) {
                                bSelectionStartPosFound = false;
                            } else {
                                vSelectionLinesToDraw.push_back(
                                    {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                            }
                        }

                        // Switch to a new line.
                        if (iLineIndex >= iLinesToSkip) {
                            screenY -= textHeightInPixels + lineSpacingInPixels;
                        }
                        screenX = leftBottomTextPos.x * iWindowWidth;

                        if (optionalSelection.has_value() && bSelectionStartPosFound) {
                            vSelectionLinesToDraw.push_back(
                                {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                        }

                        if (screenY < screenYEnd) {
                            bReachedEndOfUiNode = true;
                        }

                        iLineIndex += 1;
                    };

                    // Handle new line.
                    if (character == '\n' && pTextNode->getHandleNewLineChars()) {
                        switchToNewLine();
                        if (bReachedEndOfUiNode) {
                            break;
                        }
                        continue; // don't render \n
                    }

                    // Get glyph.
                    auto charIt = mtxLoadedGlyphs.second.find(character);
                    if (charIt == mtxLoadedGlyphs.second.end()) [[unlikely]] {
                        // No glyph found for this character, use ? instead.
                        // DON'T log a warning - you will slow everything down due to log flushing.
                        charIt = placeHolderGlythIt;
                    }
                    const auto& glyph = charIt->second;

                    const float distanceToNextGlyph =
                        (glyph.advance >> 6) * // NOLINT: bitshift by 6 to get value in pixels (2^6 = 64)
                        scale;

                    // Handle word wrap.
                    // TODO: do per-character wrap for now, rework later
                    if (pTextNode->getIsWordWrapEnabled() &&
                        (screenX + distanceToNextGlyph > screenMaxXForWordWrap)) {
                        switchToNewLine();
                        if (bReachedEndOfUiNode) {
                            break;
                        }
                    } else if (iLineIndex >= iLinesToSkip) {
                        // Check cursor.
                        if (optionalCursorOffset.has_value() && *optionalCursorOffset == iCharIndex) {
                            vCursorScreenPosToDraw.push_back(
                                CursorDrawInfo{
                                    .screenPos = glm::vec2(screenX, screenY),
                                    .height = FontManager::getFontHeightToLoad() * scale});
                        }

                        // Check selection.
                        if (optionalSelection.has_value()) {
                            if (!bSelectionStartPosFound) {
                                if (optionalSelection->first == iCharIndex) {
                                    bSelectionStartPosFound = true;
                                    vSelectionLinesToDraw.push_back(
                                        {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                                } else if (
                                    iLineIndex == iLinesToSkip && optionalSelection->first <= iCharIndex) {
                                    // Selection start was above (we skipped that line due to scroll).
                                    bSelectionStartPosFound = true;
                                    vSelectionLinesToDraw.push_back(
                                        {glm::vec2(leftBottomTextPos.x * iWindowWidth, screenY),
                                         glm::vec2(leftBottomTextPos.x * iWindowWidth, screenY)});
                                }
                            } else if (bSelectionStartPosFound && optionalSelection->second == iCharIndex) {
                                if (iLineIndex >= iLinesToSkip) {
                                    vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);
                                } else {
                                    vSelectionLinesToDraw.pop_back();
                                }
                                bSelectionStartPosFound = false;
                            }
                        }
                    }

                    if (iLineIndex >= iLinesToSkip) {
                        float xpos = screenX + glyph.bearing.x * scale;
                        float ypos = screenY - (glyph.size.y - glyph.bearing.y) * scale;

                        float width = static_cast<float>(glyph.size.x) * scale;
                        float height = static_cast<float>(glyph.size.y) * scale;

                        // Space character has 0 width so don't submit any rendering.
                        if (glyph.size.x != 0) {
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
                            iRenderedCharCount += 1;
                        }
                    }

                    // Switch to next glyph.
                    screenX += distanceToNextGlyph;
                }

                // Check cursor.
                if (optionalCursorOffset.has_value() && *optionalCursorOffset >= sText.size() &&
                    screenX < screenMaxXForWordWrap && screenY > screenYEnd && iRenderedCharCount != 0) {
                    vCursorScreenPosToDraw.push_back(
                        CursorDrawInfo{
                            .screenPos = glm::vec2(screenX, screenY),
                            .height = FontManager::getFontHeightToLoad() * scale});
                }

                // Check selection.
                if (optionalSelection.has_value()) {
                    if (bSelectionStartPosFound && optionalSelection->second >= sText.size()) {
                        vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);
                    }
                    vTextSelectionToDraw.push_back(
                        TextSelectionDrawInfo{
                            .vLineStartEndScreenPos = std::move(vSelectionLinesToDraw),
                            .textHeightInPixels = textHeightInPixels,
                            .color = selectionColor});
                }

                // Check scroll bar.
                constexpr float scrollBarWidthRelative = 0.025F; // relative to width of the UI node
                if (pTextNode->getIsScrollBarEnabled() && iCharIndex + 1 < pTextNode->getText().size()) {
                    iLinesToSkip = pTextNode->iCurrentScrollOffset;

                    const float widthInPixels =
                        scrollBarWidthRelative * pTextNode->getSize().x * iWindowWidth;
                    const auto iAverageLineCountDisplayed =
                        static_cast<size_t>(pTextNode->getSize().y * iWindowHeight / textHeightInPixels);

                    const float verticalSize = std::min(
                        1.0F,
                        static_cast<float>(iAverageLineCountDisplayed) /
                            static_cast<float>(pTextNode->iNewLineCharCountInText));

                    const float verticalPos = std::min(
                        1.0F,
                        static_cast<float>(pTextNode->iCurrentScrollOffset) /
                            static_cast<float>(pTextNode->iNewLineCharCountInText));

                    vScrollBarToDraw.push_back(
                        ScrollBarDrawInfo{
                            .posInPixels = glm::vec2(
                                screenMaxXForWordWrap - widthInPixels, leftBottomTextPos.y * iWindowHeight),
                            .widthInPixels = widthInPixels,
                            .heightInPixels = pTextNode->getSize().y * iWindowHeight,
                            .verticalPos = verticalPos,
                            .verticalSize = verticalSize,
                            .color = pTextNode->getScrollBarColor(),
                        });
                }
            }
        }

        if (!vCursorScreenPosToDraw.empty()) {
            // Draw cursors.

            // Set shader program.
            auto& pShaderProgram = mtxData.second.pRectAndCursorShaderProgram;
            if (pShaderProgram == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected the shader to be loaded at this point");
            }
            glUseProgram(pShaderProgram->getShaderProgramId());

            glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

            // Set shader parameters.
            pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);
            pShaderProgram->setVector4ToShader("color", glm::vec4(1.0F, 1.0F, 1.0F, 1.0F));
            pShaderProgram->setBoolToShader("bIsUsingTexture", false);

            for (const auto& cursorInfo : vCursorScreenPosToDraw) {
                const auto screenPos = cursorInfo.screenPos;
                const float cursorWidth = 2.0F; // NOLINT
                const float cursorHeight = cursorInfo.height * iWindowHeight;

                const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                    glm::vec4(screenPos.x, screenPos.y + cursorHeight, 0.0F, 0.0F),
                    glm::vec4(screenPos.x + cursorWidth, screenPos.y, 1.0F, 1.0F),
                    glm::vec4(screenPos.x, screenPos.y, 0.0F, 1.0F),

                    glm::vec4(screenPos.x, screenPos.y + cursorHeight, 0.0F, 0.0F),
                    glm::vec4(screenPos.x + cursorWidth, screenPos.y + cursorHeight, 1.0F, 0.0F),
                    glm::vec4(screenPos.x + cursorWidth, screenPos.y, 1.0F, 1.0F)};

                // Copy new vertex data to VBO.
                glBindBuffer(
                    GL_ARRAY_BUFFER, mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Render quad.
                glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
            }
        }

        if (!vTextSelectionToDraw.empty()) {
            // Draw selections.

            // Set shader program.
            auto& pShaderProgram = mtxData.second.pRectAndCursorShaderProgram;
            if (pShaderProgram == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected the shader to be loaded at this point");
            }
            glUseProgram(pShaderProgram->getShaderProgramId());

            glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

            // Set shader parameters.
            pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);
            pShaderProgram->setBoolToShader("bIsUsingTexture", false);

            for (auto& selectionInfo : vTextSelectionToDraw) {
                pShaderProgram->setVector4ToShader("color", selectionInfo.color);

                for (auto& [startPos, endPos] : selectionInfo.vLineStartEndScreenPos) {
                    const auto width = endPos.x - startPos.x;
                    const auto height = selectionInfo.textHeightInPixels;

                    const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                        glm::vec4(startPos.x, startPos.y + height, 0.0F, 0.0F),
                        glm::vec4(startPos.x + width, startPos.y, 1.0F, 1.0F),
                        glm::vec4(startPos.x, startPos.y, 0.0F, 1.0F),

                        glm::vec4(startPos.x, startPos.y + height, 0.0F, 0.0F),
                        glm::vec4(startPos.x + width, startPos.y + height, 1.0F, 0.0F),
                        glm::vec4(startPos.x + width, startPos.y, 1.0F, 1.0F)};

                    // Copy new vertex data to VBO.
                    glBindBuffer(
                        GL_ARRAY_BUFFER,
                        mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                    glBindBuffer(GL_ARRAY_BUFFER, 0);

                    // Render quad.
                    glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
                }
            }
        }

        if (!vScrollBarToDraw.empty()) {
            // Draw scroll bars.

            // Set shader program.
            auto& pShaderProgram = mtxData.second.pRectAndCursorShaderProgram;
            if (pShaderProgram == nullptr) [[unlikely]] {
                Error::showErrorAndThrowException("expected the shader to be loaded at this point");
            }
            glUseProgram(pShaderProgram->getShaderProgramId());

            glBindVertexArray(mtxData.second.pScreenQuadGeometry->getVao().getVertexArrayObjectId());

            // Set shader parameters.
            pShaderProgram->setMatrix4ToShader("projectionMatrix", uiProjMatrix);
            pShaderProgram->setBoolToShader("bIsUsingTexture", false);

            for (auto& scrollBarInfo : vScrollBarToDraw) {
                pShaderProgram->setVector4ToShader("color", scrollBarInfo.color);

                auto startPos = scrollBarInfo.posInPixels;
                startPos.y -= scrollBarInfo.heightInPixels * scrollBarInfo.verticalPos;
                startPos.y += (1.0F - scrollBarInfo.verticalSize) * scrollBarInfo.heightInPixels;

                const auto width = scrollBarInfo.widthInPixels;
                const auto height = scrollBarInfo.heightInPixels * scrollBarInfo.verticalSize;

                const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
                    glm::vec4(startPos.x, startPos.y + height, 0.0F, 0.0F),
                    glm::vec4(startPos.x + width, startPos.y, 1.0F, 1.0F),
                    glm::vec4(startPos.x, startPos.y, 0.0F, 1.0F),

                    glm::vec4(startPos.x, startPos.y + height, 0.0F, 0.0F),
                    glm::vec4(startPos.x + width, startPos.y + height, 1.0F, 0.0F),
                    glm::vec4(startPos.x + width, startPos.y, 1.0F, 1.0F)};

                // Copy new vertex data to VBO.
                glBindBuffer(
                    GL_ARRAY_BUFFER, mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                // Render quad.
                glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
            }
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_BLEND);
}

void UiManager::changeFocusedNode(UiNode* pNode) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode == pNode) {
        return;
    }

    if (mtxData.second.pFocusedNode != nullptr) {
        mtxData.second.pFocusedNode->onLostFocus();
    }

    mtxData.second.pFocusedNode = pNode;

    if (mtxData.second.pFocusedNode != nullptr) {
        mtxData.second.pFocusedNode->onGainedFocus();
    }
}

UiManager::~UiManager() {
    std::scoped_lock guard(mtxData.first);

    mtxData.second.pRectAndCursorShaderProgram = nullptr;

    if (mtxData.second.pFocusedNode != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "UI manager is being destroyed but focused node pointer is still not `nullptr`");
    }
    if (mtxData.second.pHoveredNodeLastFrame != nullptr) [[unlikely]] {
        Error::showErrorAndThrowException(
            "UI manager is being destroyed but hovered node pointer is still not `nullptr`");
    }
    if (!mtxData.second.modalInputReceivingNodes.empty()) [[unlikely]] {
        Error::showErrorAndThrowException(
            "UI manager is being destroyed but array of modal nodes is still not empty");
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
