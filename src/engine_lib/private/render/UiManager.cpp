#include "render/UiManager.h"

// Standard.
#include <format>
#include <ranges>

// Custom.
#include "game/node/ui/TextUiNode.h"
#include "game/node/ui/TextEditUiNode.h"
#include "game/node/ui/RectUiNode.h"
#include "game/node/ui/SliderUiNode.h"
#include "game/node/ui/LayoutUiNode.h"
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
        Error::showErrorAndThrowException(std::format(                                                       \
            "unable to find the node \"{}\" with depth {} to remove from rendering",                         \
            pNode->getNodeName(),                                                                            \
            iNodeDepth));                                                                                    \
    }                                                                                                        \
    const auto iArrayIndex = *optionalArrayIndex;                                                            \
                                                                                                             \
    vNodesByDepth[iArrayIndex].second.erase(pNode);                                                          \
    if (vNodesByDepth[iArrayIndex].second.empty()) {                                                         \
        vNodesByDepth.erase(vNodesByDepth.begin() + static_cast<long>(iArrayIndex));                         \
    }

void UiManager::onNodeSpawning(TextUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vTextNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(TextUiNode);
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

void UiManager::onNodeSpawning(SliderUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& data = mtxData.second;
    auto& vNodesByDepth = data.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vSliderNodes;

    if (!pNode->isVisible()) {
        return;
    }

    ADD_NODE_TO_RENDERING(SliderUiNode);
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

void UiManager::onSpawnedNodeChangedVisibility(SliderUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vSliderNodes;

    if (pNode->isVisible()) {
        ADD_NODE_TO_RENDERING(SliderUiNode);
    } else {
        REMOVE_NODE_FROM_RENDERING(SliderUiNode);
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

void UiManager::onNodeDespawning(SliderUiNode* pNode) {
    std::scoped_lock guard(mtxData.first);
    auto& vNodesByDepth =
        mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vSliderNodes;

    if (!pNode->isVisible()) {
        return;
    }

    REMOVE_NODE_FROM_RENDERING(SliderUiNode);
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
    } else if (auto pNode = dynamic_cast<SliderUiNode*>(pTargetNode)) {
        auto& vNodesByDepth =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())].vSliderNodes;
        {
            REMOVE_NODE_FROM_RENDERING(SliderUiNode);
        }
        {
            ADD_NODE_TO_RENDERING(SliderUiNode);
        }
    } else [[unlikely]] {
        Error::showErrorAndThrowException("unhandled case");
    }

#if defined(WIN32) && defined(DEBUG)
    static_assert(
        sizeof(Data::SpawnedVisibleLayerUiNodes) == 224, "add new variables here"); // NOLINT: current size
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
            Error::showErrorAndThrowException(std::format(
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
        Error::showErrorAndThrowException(std::format(
            "unable to find node \"{}\" to be spawned, visible and receiving input to make focused",
            pFocusedNode->getNodeName()));
    }

    changeFocusedNode(pFocusedNode);
}

void UiManager::onSpawnedUiNodeInputStateChange(UiNode* pNode, bool bEnabledInput) {
    std::scoped_lock guard(mtxData.first);
    auto& layerNodes = mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())];
    auto& nodes = layerNodes.receivingInputUiNodes;

    if (bEnabledInput) {
        if (!nodes.insert(pNode).second) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "spawned node \"{}\" enabled input but it already exists in UI manager's array of nodes "
                "that receive input",
                pNode->getNodeName()));
        }
        if (auto pLayoutNode = dynamic_cast<LayoutUiNode*>(pNode)) {
            if (!layerNodes.layoutNodesWithScrollBars.insert(pLayoutNode).second) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "spawned layout node \"{}\" enabled input but it already exists in UI manager's "
                    "array of nodes that receive input",
                    pNode->getNodeName()));
            }
        }
    } else {
        const auto it = nodes.find(pNode);
        if (it == nodes.end()) [[unlikely]] {
            Error::showErrorAndThrowException(std::format(
                "unable to find spawned node \"{}\" to remove from the array of nodes that receive input",
                pNode->getNodeName()));
        }
        nodes.erase(it);

        if (auto pLayoutNode = dynamic_cast<LayoutUiNode*>(pNode)) {
            const auto layoutIt = layerNodes.layoutNodesWithScrollBars.find(pLayoutNode);
            if (layoutIt == layerNodes.layoutNodesWithScrollBars.end()) [[unlikely]] {
                Error::showErrorAndThrowException(std::format(
                    "unable to find spawned layout \"{}\" to remove from the array of nodes that receive "
                    "input",
                    pNode->getNodeName()));
            }
            layerNodes.layoutNodesWithScrollBars.erase(layoutIt);
        }

        // Remove from rendered last frame in order to avoid triggering input on node after it was despawned.
        auto& vInputNodesRenderedLastFrame =
            mtxData.second.vSpawnedVisibleNodes[static_cast<size_t>(pNode->getUiLayer())]
                .receivingInputUiNodesRenderedLastFrame;
        for (size_t i = 0; i < vInputNodesRenderedLastFrame.size(); i++) {
            if (vInputNodesRenderedLastFrame[i] == pNode) {
                vInputNodesRenderedLastFrame.erase(
                    vInputNodesRenderedLastFrame.begin() + static_cast<long>(i));
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

void UiManager::onGamepadInput(GamepadButton button, bool bIsPressedDown) {
    std::scoped_lock guard(mtxData.first);

    if (mtxData.second.pFocusedNode == nullptr) {
        return;
    }

    mtxData.second.pFocusedNode->onGamepadInputWhileFocused(button, bIsPressedDown);
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
        static_cast<float>(iCursorY) / static_cast<float>(iHeight));

    if (!mtxData.second.modalInputReceivingNodes.empty()) {
        for (auto& pNode : mtxData.second.modalInputReceivingNodes) {
            const auto pos = pNode->getPosition();
            const auto size = pNode->getSize();

            if (cursorPos.y < pos.y) {
                continue;
            }
            if (cursorPos.x < pos.x) {
                continue;
            }
            if (cursorPos.y > pos.y + size.y) {
                continue;
            }
            if (cursorPos.x > pos.x + size.x) {
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
            const auto pos = pNode->getPosition();
            const auto size = pNode->getSize();

            if (cursorPos.y < pos.y) {
                continue;
            }
            if (cursorPos.x < pos.x) {
                continue;
            }
            if (cursorPos.y > pos.y + size.y) {
                continue;
            }
            if (cursorPos.x > pos.x + size.x) {
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
        static_cast<float>(iCursorY) / static_cast<float>(iHeight));

    bool bFoundNode = false;

    if (!mtxData.second.modalInputReceivingNodes.empty()) {
        for (auto& pNode : mtxData.second.modalInputReceivingNodes) {
            const auto pos = pNode->getPosition();
            const auto size = pNode->getSize();

            if (cursorPos.y < pos.y) {
                continue;
            }
            if (cursorPos.x < pos.x) {
                continue;
            }
            if (cursorPos.y > pos.y + size.y) {
                continue;
            }
            if (cursorPos.x > pos.x + size.x) {
                continue;
            }

            pNode->bIsHoveredCurrFrame = true;
            mtxData.second.pHoveredNodeLastFrame = pNode;

            if (!pNode->bIsHoveredPrevFrame) {
                pNode->onMouseEntered();
            }

            // When there's a model UI we must send mouse move (not the game manager).
            pNode->onMouseMove(static_cast<double>(iXOffset), static_cast<double>(iYOffset));
            break;
        }
    } else {
        // Check rendered input nodes in reverse order (from front layer to back layer).
        for (const auto& layerNodes : std::views::reverse(mtxData.second.vSpawnedVisibleNodes)) {
            for (auto& pNode : layerNodes.receivingInputUiNodesRenderedLastFrame) {
                const auto pos = pNode->getPosition();
                const auto size = pNode->getSize();

                if (cursorPos.y < pos.y) {
                    continue;
                }
                if (cursorPos.x < pos.x) {
                    continue;
                }
                if (cursorPos.y > pos.y + size.y) {
                    continue;
                }
                if (cursorPos.x > pos.x + size.x) {
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

    // Load shaders.
    mtxData.second.pRectAndCursorShaderProgram = pRenderer->getShaderManager().getShaderProgram(
        "engine/shaders/ui/UiScreenQuad.vert.glsl",
        "engine/shaders/ui/RectUiNode.frag.glsl",
        ShaderProgramUsage::OTHER);
    mtxData.second.pTextShaderProgram = pRenderer->getShaderManager().getShaderProgram(
        "engine/shaders/ui/UiScreenQuad.vert.glsl",
        "engine/shaders/ui/TextNode.frag.glsl",
        ShaderProgramUsage::OTHER);
}

void UiManager::onWindowSizeChanged() {
    const auto [iWidth, iHeight] = pRenderer->getWindow()->getWindowSize();
    uiProjMatrix = glm::ortho(0.0F, static_cast<float>(iWidth), 0.0F, static_cast<float>(iHeight));
}

void UiManager::drawUiOnFramebuffer(unsigned int iDrawFramebufferId) {
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
        drawSliderNodes(i);
        drawLayoutScrollBars(i);
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

                pos.y += size.y * pRectNode->clipY.x;
                size.y = size.y * (pRectNode->clipY.y - pRectNode->clipY.x);

                // Set shader parameters.
                pShaderProgram->setVector4ToShader("color", pRectNode->getColor());
                if (pRectNode->pTexture != nullptr) {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", true);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, pRectNode->pTexture->getTextureId());
                } else {
                    pShaderProgram->setBoolToShader("bIsUsingTexture", false);
                }

                pos = glm::vec2(
                    pos.x * static_cast<float>(iWindowWidth), pos.y * static_cast<float>(iWindowHeight));
                size = glm::vec2(
                    size.x * static_cast<float>(iWindowWidth), size.y * static_cast<float>(iWindowHeight));

                drawQuad(pos, size, iWindowHeight, pRectNode->clipY);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawSliderNodes(size_t iLayer) {
    PROFILE_FUNC;

    std::scoped_lock guard(mtxData.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vSliderNodes;
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
        pShaderProgram->setBoolToShader("bIsUsingTexture", false);

        constexpr float sliderHeightToWidthRatio = 0.5F; // NOLINT
        constexpr float sliderHandleWidth = 0.1F; // NOLINT: in range [0.0; 1.0] relative to slider width

        for (const auto& [iDepth, nodes] : vNodesByDepth) {
            for (const auto& pSliderNode : nodes) {
                // Update input-related things.
                if (pSliderNode->isReceivingInputUnsafe()) { // safe - node won't despawn/change state here
                                                             // (it will wait on our mutex)
                    vInputNodesRendered.push_back(pSliderNode);
                }
                if (!pSliderNode->bIsHoveredCurrFrame && pSliderNode->bIsHoveredPrevFrame) {
                    pSliderNode->onMouseLeft();
                }
                pSliderNode->bIsHoveredPrevFrame = pSliderNode->bIsHoveredCurrFrame;
                pSliderNode->bIsHoveredCurrFrame = false;

                const auto pos = pSliderNode->getPosition();
                const auto size = pSliderNode->getSize();
                const auto handlePos = pSliderNode->getHandlePosition();

                // Draw slider base.
                pShaderProgram->setVector4ToShader("color", pSliderNode->getSliderColor());
                const auto baseHeight = size.y * sliderHeightToWidthRatio;
                drawQuad(
                    glm::vec2(
                        pos.x * static_cast<float>(iWindowWidth),
                        (pos.y + size.y / 2.0F - baseHeight / 2.0F) * static_cast<float>(iWindowHeight)),
                    glm::vec2(
                        size.x * static_cast<float>(iWindowWidth),
                        baseHeight * static_cast<float>(iWindowHeight)),
                    iWindowHeight);

                // Draw slider handle.
                pShaderProgram->setVector4ToShader("color", pSliderNode->getSliderHandleColor());
                const auto handleWidth = size.x * sliderHandleWidth;
                const auto handleCenterPos = glm::vec2(pos.x + handlePos * size.x, pos.y);
                drawQuad(
                    glm::vec2(
                        (handleCenterPos.x - handleWidth / 2.0F) * static_cast<float>(iWindowWidth),
                        handleCenterPos.y * static_cast<float>(iWindowHeight)),
                    glm::vec2(
                        handleWidth * static_cast<float>(iWindowWidth),
                        size.y * static_cast<float>(iWindowHeight)),
                    iWindowHeight);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawTextNodes(size_t iLayer) { // NOLINT
    PROFILE_FUNC;

    auto& fontManager = pRenderer->getFontManager();
    auto glyphGuard = fontManager.getGlyphs();
    std::scoped_lock guard(mtxData.first);

    auto& vNodesByDepth = mtxData.second.vSpawnedVisibleNodes[iLayer].vTextNodes;
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
        std::vector<ScrollBarDrawInfo> vScrollBarToDraw;

        enum class SelectionDrawState { LOOKING_FOR_START, LOOKING_FOR_END, FINISHED };

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
                std::vector<std::pair<glm::vec2, glm::vec2>> vSelectionLinesToDraw;
                SelectionDrawState selectionDrawState = SelectionDrawState::LOOKING_FOR_START;

                // Prepare some variables for rendering.
                const auto sText = pTextNode->getText();
                const auto textPos = pTextNode->getPosition();
                const auto screenMaxXForWordWrap =
                    (textPos.x + pTextNode->getSize().x) * static_cast<float>(iWindowWidth);

                float screenX = textPos.x * static_cast<float>(iWindowWidth);
                float screenY = textPos.y * static_cast<float>(iWindowHeight);
                const auto screenYEnd = screenY + pTextNode->getSize().y * static_cast<float>(iWindowHeight);
                const auto scale = pTextNode->getTextHeight() / fontManager.getFontHeightToLoad();

                const float textHeightInPixels =
                    static_cast<float>(iWindowHeight) * fontManager.getFontHeightToLoad() * scale;
                const float lineSpacingInPixels = pTextNode->getTextLineSpacing() * textHeightInPixels;

                // Check scroll bar.
                size_t iLinesToSkip = 0;
                if (pTextNode->getIsScrollBarEnabled()) {
                    iLinesToSkip = pTextNode->getCurrentScrollOffset();
                }

                // Set color.
                pShaderProgram->setVector4ToShader("textColor", pTextNode->getTextColor());

                // Switch to the first row of text.
                screenY += textHeightInPixels;

                // Render each character.
                size_t iLineIndex = 0;
                size_t iRenderedCharCount = 0;
                size_t iCharIndex = 0;
                bool bReachedEndOfUiNode = false;
                for (; iCharIndex < sText.size(); iCharIndex++) {
                    const auto& character = sText[iCharIndex];

                    // Prepare a handy lambda.
                    const auto switchToNewLine = [&]() {
                        // Check cursor.
                        if (optionalCursorOffset.has_value() && *optionalCursorOffset == iCharIndex) {
                            vCursorScreenPosToDraw.push_back(CursorDrawInfo{
                                .screenPos = glm::vec2(screenX, screenY),
                                .height = fontManager.getFontHeightToLoad() * scale});
                        }

                        // Check selection.
                        bool bStartNewSelectionRegionOnNewLine = false;
                        if (optionalSelection.has_value() &&
                            selectionDrawState == SelectionDrawState::LOOKING_FOR_END) {
                            vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);

                            if (optionalSelection->second == iCharIndex) {
                                selectionDrawState = SelectionDrawState::FINISHED;
                            } else {
                                vSelectionLinesToDraw.push_back(
                                    {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                                bStartNewSelectionRegionOnNewLine = true;
                            }
                        }

                        // Switch to a new line.
                        if (iLineIndex >= iLinesToSkip) {
                            screenY += textHeightInPixels + lineSpacingInPixels;
                        }
                        screenX = textPos.x * static_cast<float>(iWindowWidth);

                        if (bStartNewSelectionRegionOnNewLine) {
                            vSelectionLinesToDraw.push_back(
                                {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                        }

                        if (screenY > screenYEnd) {
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

                    const auto& glyph = glyphGuard.getGlyph(character);

                    const float distanceToNextGlyph =
                        static_cast<float>(
                            glyph.advance >> 6) * // bitshift by 6 to get value in pixels (2^6 = 64)
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
                            vCursorScreenPosToDraw.push_back(CursorDrawInfo{
                                .screenPos = glm::vec2(screenX, screenY),
                                .height = fontManager.getFontHeightToLoad() * scale});
                        }

                        // Check selection.
                        if (optionalSelection.has_value() &&
                            selectionDrawState != SelectionDrawState::FINISHED) {
                            if (selectionDrawState == SelectionDrawState::LOOKING_FOR_START) {
                                if (optionalSelection->first == iCharIndex) {
                                    selectionDrawState = SelectionDrawState::LOOKING_FOR_END;
                                    vSelectionLinesToDraw.push_back(
                                        {glm::vec2(screenX, screenY), glm::vec2(screenX, screenY)});
                                } else if (
                                    iLineIndex == iLinesToSkip && optionalSelection->first <= iCharIndex) {
                                    // Selection start was above (we skipped that line due to scroll).
                                    selectionDrawState = SelectionDrawState::LOOKING_FOR_END;
                                    vSelectionLinesToDraw.push_back(
                                        {glm::vec2(textPos.x * static_cast<float>(iWindowWidth), screenY),
                                         glm::vec2(textPos.x * static_cast<float>(iWindowWidth), screenY)});
                                }
                            } else if (
                                selectionDrawState == SelectionDrawState::LOOKING_FOR_END &&
                                optionalSelection->second == iCharIndex) {
                                if (iLineIndex >= iLinesToSkip) {
                                    vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);
                                } else {
                                    vSelectionLinesToDraw.pop_back();
                                }
                                selectionDrawState = SelectionDrawState::FINISHED;
                            }
                        }
                    }

                    if (iLineIndex >= iLinesToSkip) {
                        float xpos = screenX + static_cast<float>(glyph.bearing.x) * scale;
                        float ypos = screenY - static_cast<float>(glyph.bearing.y) * scale;

                        float width = static_cast<float>(glyph.size.x) * scale;
                        float height = static_cast<float>(glyph.size.y) * scale;

                        // Space character has 0 width so don't submit any rendering.
                        if (glyph.size.x != 0) {
                            glBindTexture(GL_TEXTURE_2D, glyph.pTexture->getTextureId());
                            drawQuad(glm::vec2(xpos, ypos), glm::vec2(width, height), iWindowHeight);
                            iRenderedCharCount += 1;
                        }
                    }

                    // Switch to next glyph.
                    screenX += distanceToNextGlyph;
                }

                // Check cursor.
                if (optionalCursorOffset.has_value() && *optionalCursorOffset >= sText.size() &&
                    screenX < screenMaxXForWordWrap && screenY < screenYEnd && iRenderedCharCount != 0) {
                    vCursorScreenPosToDraw.push_back(CursorDrawInfo{
                        .screenPos = glm::vec2(screenX, screenY),
                        .height = fontManager.getFontHeightToLoad() * scale});
                }

                // Check selection.
                if (optionalSelection.has_value() && !vSelectionLinesToDraw.empty()) {
                    if (selectionDrawState == SelectionDrawState::LOOKING_FOR_END &&
                        optionalSelection->second >= sText.size()) {
                        vSelectionLinesToDraw.back().second = glm::vec2(screenX, screenY);
                    }
                    vTextSelectionToDraw.push_back(TextSelectionDrawInfo{
                        .vLineStartEndScreenPos = std::move(vSelectionLinesToDraw),
                        .textHeightInPixels = textHeightInPixels,
                        .color = selectionColor});
                }

                // Check scroll bar.
                if (pTextNode->getIsScrollBarEnabled()) {
                    const float widthInPixels = scrollBarWidthRelativeNode * pTextNode->getSize().x *
                                                static_cast<float>(iWindowWidth);
                    const auto iAverageLineCountDisplayed = static_cast<size_t>(
                        pTextNode->getSize().y * static_cast<float>(iWindowHeight) / textHeightInPixels);

                    const float verticalSize = std::min(
                        1.0F,
                        static_cast<float>(iAverageLineCountDisplayed) /
                            static_cast<float>(pTextNode->iNewLineCharCountInText));

                    const float verticalPos = std::min(
                        1.0F,
                        static_cast<float>(pTextNode->iCurrentScrollOffset) /
                            static_cast<float>(
                                std::max(pTextNode->iNewLineCharCountInText, static_cast<size_t>(1))));

                    vScrollBarToDraw.push_back(ScrollBarDrawInfo{
                        .posInPixels = glm::vec2(
                            screenMaxXForWordWrap - widthInPixels,
                            textPos.y * static_cast<float>(iWindowHeight)),
                        .widthInPixels = widthInPixels,
                        .heightInPixels = pTextNode->getSize().y * static_cast<float>(iWindowHeight),
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
                const float cursorWidth = 2.0F; // NOLINT
                const float cursorHeight = cursorInfo.height * static_cast<float>(iWindowHeight);
                const auto screenPos = glm::vec2(
                    cursorInfo.screenPos.x, cursorInfo.screenPos.y - cursorHeight); // to draw from top

                drawQuad(screenPos, glm::vec2(cursorWidth, cursorHeight), iWindowHeight);
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
                    const auto pos = glm::vec2(startPos.x, startPos.y - height); // to draw from top

                    drawQuad(pos, glm::vec2(width, height), iWindowHeight);
                }
            }
        }

        if (!vScrollBarToDraw.empty()) {
            drawScrollBarsDataLocked(vScrollBarToDraw, iWindowHeight);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawLayoutScrollBars(size_t iLayer) {
    std::scoped_lock guard(mtxData.first);
    auto& layerNodes = mtxData.second.vSpawnedVisibleNodes[iLayer];
    if (layerNodes.layoutNodesWithScrollBars.empty()) {
        return;
    }

    const auto [iWindowWidth, iWindowHeight] = pRenderer->getWindow()->getWindowSize();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    {
        auto& renderedNodes = layerNodes.receivingInputUiNodesRenderedLastFrame;

        std::vector<ScrollBarDrawInfo> vScrollBarsToDraw;
        vScrollBarsToDraw.reserve(layerNodes.layoutNodesWithScrollBars.size());

        for (const auto& pLayoutNode : layerNodes.layoutNodesWithScrollBars) {
            renderedNodes.push_back(pLayoutNode);

            const auto nodePos = pLayoutNode->getPosition();
            const auto nodeSize = pLayoutNode->getSize();

            const float widthInPixels =
                scrollBarWidthRelativeNode * nodeSize.x * static_cast<float>(iWindowWidth);
            const auto posInPixels = glm::vec2(
                (nodePos.x + nodeSize.x) * static_cast<float>(iWindowWidth) - widthInPixels,
                nodePos.y * static_cast<float>(iWindowHeight));

            float verticalSize = 1.0F / pLayoutNode->totalScrollHeight;
            if (pLayoutNode->totalScrollHeight < 1.0F) {
                verticalSize = 1.0F;
            }
            const float verticalPos = std::min(
                1.0F,
                (static_cast<float>(pLayoutNode->iCurrentScrollOffset) * LayoutUiNode::scrollBarStepLocal) /
                    pLayoutNode->totalScrollHeight);

            vScrollBarsToDraw.push_back(ScrollBarDrawInfo{
                .posInPixels = posInPixels,
                .widthInPixels = widthInPixels,
                .heightInPixels = pLayoutNode->getSize().y * static_cast<float>(iWindowHeight),
                .verticalPos = verticalPos,
                .verticalSize = verticalSize,
                .color = pLayoutNode->getScrollBarColor(),
            });
        }

        drawScrollBarsDataLocked(vScrollBarsToDraw, iWindowHeight);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_BLEND);
}

void UiManager::drawScrollBarsDataLocked(
    const std::vector<ScrollBarDrawInfo>& vScrollBarsToDraw, unsigned int iWindowHeight) {
    if (vScrollBarsToDraw.empty()) [[unlikely]] {
        Error::showErrorAndThrowException("expected at least 1 scroll bar to be specified");
    }

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

    for (auto& scrollBarInfo : vScrollBarsToDraw) {
        pShaderProgram->setVector4ToShader("color", scrollBarInfo.color);

        const auto width = scrollBarInfo.widthInPixels;
        auto height = scrollBarInfo.heightInPixels * scrollBarInfo.verticalSize;
        auto pos = scrollBarInfo.posInPixels;
        pos.y += scrollBarInfo.verticalPos * scrollBarInfo.heightInPixels;

        // TODO: scroll bar goes under the UI node sometimes when near end of the text
        if (scrollBarInfo.verticalPos + scrollBarInfo.verticalSize > 1.0F) {
            height = (1.0F - scrollBarInfo.verticalPos) * scrollBarInfo.heightInPixels;
        }

        drawQuad(pos, glm::vec2(width, height), iWindowHeight);
    }
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

void UiManager::drawQuad(
    const glm::vec2& screenPos,
    const glm::vec2& screenSize,
    unsigned int iScreenHeight,
    const glm::vec2& yClip) const {
    // Flip Y from our top-left origin to OpenGL's bottom-left origin.
    const float posY = static_cast<float>(iScreenHeight) - screenPos.y;

    // Update vertices.
    const std::array<glm::vec4, ScreenQuadGeometry::iVertexCount> vVertices = {
        glm::vec4(screenPos.x, posY, 0.0F, yClip.x),
        glm::vec4(screenPos.x + screenSize.x, posY - screenSize.y, 1.0F, yClip.y),
        glm::vec4(screenPos.x, posY - screenSize.y, 0.0F, yClip.y),

        glm::vec4(screenPos.x, posY, 0.0F, yClip.x),
        glm::vec4(screenPos.x + screenSize.x, posY, 1.0F, yClip.x),
        glm::vec4(screenPos.x + screenSize.x, posY - screenSize.y, 1.0F, yClip.y),
    };

    // Copy new vertex data to VBO.
    glBindBuffer(GL_ARRAY_BUFFER, mtxData.second.pScreenQuadGeometry->getVao().getVertexBufferObjectId());
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vVertices), vVertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render quad.
    glDrawArrays(GL_TRIANGLES, 0, ScreenQuadGeometry::iVertexCount);
}

UiManager::~UiManager() {
    std::scoped_lock guard(mtxData.first);

    mtxData.second.pRectAndCursorShaderProgram = nullptr;
    mtxData.second.pTextShaderProgram = nullptr;

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
        Error::showErrorAndThrowException(std::format(
            "UI manager is being destroyed but there are still {} spawned and visible nodes", iNodeCount));
    }
}
