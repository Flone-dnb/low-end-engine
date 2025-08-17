#include "io/GltfImporter.h"

// Standard.
#include <format>

// Custom.
#include "misc/ProjectPaths.h"
#include "game/node/MeshNode.h"
#include "material/TextureManager.h"

// External.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

namespace {
    constexpr std::string_view sTexturesDirNameSuffix = "_tex";
    constexpr std::string_view sImportedImageExtension = "png";
    constexpr std::string_view sDiffuseTextureName = "diffuse";
}

inline bool writeGltfTextureToDisk(const tinygltf::Image& image, const std::string& sPathRelativeResToImage) {
    auto optionalError = TextureManager::importTextureFromMemory(
        sPathRelativeResToImage,
        image.image,
        static_cast<unsigned int>(image.width),
        static_cast<unsigned int>(image.height),
        static_cast<unsigned int>(image.component));
    if (optionalError.has_value()) [[unlikely]] {
        Logger::get().error(optionalError->getFullErrorMessage());
        return false;
    }

    return true;
}

inline std::variant<Error, std::vector<std::unique_ptr<MeshNode>>> processGltfMesh(
    const tinygltf::Model& model,
    const tinygltf::Mesh& mesh,
    const std::filesystem::path& pathToOutputFile,
    const std::string& sPathToOutputDirRelativeRes,
    const std::function<void(std::string_view)>& onProgress,
    size_t& iGltfNodeProcessedCount,
    const size_t iTotalGltfNodesToProcess) {
    // Prepare array to fill.
    std::vector<std::unique_ptr<MeshNode>> vMeshNodes;

    // Prepare textures directory.
    const std::string sFilename = pathToOutputFile.stem().string();
    const std::filesystem::path pathToTexturesDir =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToOutputDirRelativeRes /
        (sFilename + std::string(sTexturesDirNameSuffix));
    if (std::filesystem::exists(pathToTexturesDir)) {
        std::filesystem::remove_all(pathToTexturesDir);
    }

    // Go through each mesh in this node.
    for (size_t iPrimitive = 0; iPrimitive < mesh.primitives.size(); iPrimitive++) {
        auto& primitive = mesh.primitives[iPrimitive];

        // Mark progress.
        onProgress(std::format(
            "processing GLTF node {}/{}, mesh {}/{}",
            iGltfNodeProcessedCount,
            iTotalGltfNodesToProcess,
            iPrimitive,
            mesh.primitives.size()));

        MeshGeometry meshGeometry;

        {
            // Add indices.
            // Save accessor to mesh indices.
            const auto& indexAccessor = model.accessors[static_cast<size_t>(primitive.indices)];

            // Get index buffer.
            const auto& indexBufferView = model.bufferViews[static_cast<size_t>(indexAccessor.bufferView)];
            const auto& indexBuffer = model.buffers[static_cast<size_t>(indexBufferView.buffer)];

            // Make sure index is stored as scalar.
            if (indexAccessor.type != TINYGLTF_TYPE_SCALAR) [[unlikely]] {
                return Error(std::format(
                    "expected indices of mesh to be stored as `scalar`, actual type: {}",
                    indexAccessor.type));
            }

            // Prepare variables to read indices.
            auto pCurrentIndex =
                indexBuffer.data.data() + indexBufferView.byteOffset + indexAccessor.byteOffset;

            meshGeometry.getIndices().resize(indexAccessor.count);

            // Add indices depending on their type.
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                return Error(
                    "mesh indices have type `unsigned int` while the engine only supports `unsigned short`");

                using index_t = unsigned int;

                const auto iStride =
                    indexBufferView.byteStride == 0 ? sizeof(index_t) : indexBufferView.byteStride;

                // Set indices.
                for (size_t i = 0; i < meshGeometry.getIndices().size(); i++) {
                    // Set value.
                    meshGeometry.getIndices()[i] = static_cast<MeshGeometry::MeshIndexType>(
                        reinterpret_cast<const index_t*>(pCurrentIndex)[0]);

                    // Switch to the next item.
                    pCurrentIndex += iStride;
                }
            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) { // NOLINT
                using index_t = unsigned short;

                const auto iStride =
                    indexBufferView.byteStride == 0 ? sizeof(index_t) : indexBufferView.byteStride;

                // Set indices.
                for (size_t i = 0; i < meshGeometry.getIndices().size(); i++) {
                    // Set value.
                    meshGeometry.getIndices()[i] = static_cast<MeshGeometry::MeshIndexType>(
                        reinterpret_cast<const index_t*>(pCurrentIndex)[0]);

                    // Switch to the next item.
                    pCurrentIndex += iStride;
                }
            } else {
                return Error(std::format(
                    "expected indices mesh component type to be `unsigned int` or `unsigned short`, "
                    "actual type: {}",
                    indexAccessor.componentType));
            }
        }

        {
            // Find a position attribute to know how much vertices there will be.
            const auto it = primitive.attributes.find("POSITION");
            if (it == primitive.attributes.end()) [[unlikely]] {
                return Error("a GLTF mesh node does not have any positions defined");
            }
            const auto iPositionAccessorIndex = it->second;

            // Get accessor.
            const auto& positionAccessor = model.accessors[static_cast<size_t>(iPositionAccessorIndex)];

            // Allocate vertices.
            meshGeometry.getVertices().resize(positionAccessor.count);
        }

        // Process attributes.
        for (auto& [sAttributeName, iAccessorIndex] : primitive.attributes) {
            // Get attribute accessor.
            const auto& attributeAccessor = model.accessors[static_cast<size_t>(iAccessorIndex)];

            // Get buffer.
            const auto& attributeBufferView =
                model.bufferViews[static_cast<size_t>(attributeAccessor.bufferView)];
            const auto& attributeBuffer = model.buffers[static_cast<size_t>(attributeBufferView.buffer)];

            if (sAttributeName == "POSITION") {
                using position_t = glm::vec3;

                // Make sure position is stored as `vec3`.
                if (attributeAccessor.type != TINYGLTF_TYPE_VEC3) [[unlikely]] {
                    return Error(std::format(
                        "expected POSITION mesh attribute to be stored as `vec3`, actual type: {}",
                        attributeAccessor.type));
                }
                // Make sure that component type is `float`.
                if (attributeAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) [[unlikely]] {
                    return Error(std::format(
                        "expected POSITION mesh attribute component type to be `float`, actual type: {}",
                        attributeAccessor.componentType));
                }

                // Prepare variables.
                auto pCurrentPosition = attributeBuffer.data.data() + attributeBufferView.byteOffset +
                                        attributeAccessor.byteOffset;
                const auto iStride =
                    attributeBufferView.byteStride == 0 ? sizeof(position_t) : attributeBufferView.byteStride;

                // Set positions to mesh data.
                for (size_t i = 0; i < meshGeometry.getVertices().size(); i++) {
                    // Set value.
                    meshGeometry.getVertices()[i].position =
                        reinterpret_cast<const position_t*>(pCurrentPosition)[0];

                    // Switch to the next item.
                    pCurrentPosition += iStride;
                }

                // Process next attribute.
                continue;
            }

            if (sAttributeName == "NORMAL") {
                using normal_t = glm::vec3;

                // Make sure normal is stored as `vec3`.
                if (attributeAccessor.type != TINYGLTF_TYPE_VEC3) [[unlikely]] {
                    return Error(std::format(
                        "expected NORMAL mesh attribute to be stored as `vec3`, actual type: {}",
                        attributeAccessor.type));
                }
                // Make sure that component type is `float`.
                if (attributeAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) [[unlikely]] {
                    return Error(std::format(
                        "expected NORMAL mesh attribute component type to be `float`, actual type: {}",
                        attributeAccessor.componentType));
                }

                // Prepare variables.
                auto pCurrentNormal = attributeBuffer.data.data() + attributeBufferView.byteOffset +
                                      attributeAccessor.byteOffset;
                const auto iStride =
                    attributeBufferView.byteStride == 0 ? sizeof(normal_t) : attributeBufferView.byteStride;

                // Set normals to mesh data.
                for (size_t i = 0; i < meshGeometry.getVertices().size(); i++) {
                    // Set value.
                    meshGeometry.getVertices()[i].normal =
                        reinterpret_cast<const normal_t*>(pCurrentNormal)[0];

                    // Switch to the next item.
                    pCurrentNormal += iStride;
                }

                // Process next attribute.
                continue;
            }

            if (sAttributeName == "TEXCOORD_0") {
                using uv_t = glm::vec2;

                // Make sure UV is stored as `vec2`.
                if (attributeAccessor.type != TINYGLTF_TYPE_VEC2) [[unlikely]] {
                    return Error(std::format(
                        "expected TEXCOORD mesh attribute to be stored as `vec2`, actual type: {}",
                        attributeAccessor.type));
                }
                // Make sure that component type is `float`.
                if (attributeAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) [[unlikely]] {
                    return Error(std::format(
                        "expected TEXCOORD mesh attribute component type to be `float`, actual type: {}",
                        attributeAccessor.componentType));
                }

                // Prepare variables.
                auto pCurrentUv = attributeBuffer.data.data() + attributeBufferView.byteOffset +
                                  attributeAccessor.byteOffset;
                const auto iStride =
                    attributeBufferView.byteStride == 0 ? sizeof(uv_t) : attributeBufferView.byteStride;

                // Set UVs to mesh data.
                for (size_t i = 0; i < meshGeometry.getVertices().size(); i++) {
                    // Set value.
                    meshGeometry.getVertices()[i].uv = reinterpret_cast<const uv_t*>(pCurrentUv)[0];

                    // Switch to the next item.
                    pCurrentUv += iStride;
                }

                // Process next attribute.
                continue;
            }
        }

        // Make sure something generated.
        if (meshGeometry.getVertices().empty() || meshGeometry.getIndices().empty()) {
            continue;
        }

        // Create a new mesh node with the specified data.
        auto pMeshNode = std::make_unique<MeshNode>(!mesh.name.empty() ? mesh.name : "Mesh Node");
        pMeshNode->setMeshGeometryBeforeSpawned(std::move(meshGeometry));

        if (primitive.material >= 0) {
            // Process material.
            auto& material = model.materials[static_cast<size_t>(primitive.material)];
            auto& meshMaterial = pMeshNode->getMaterial();

            // IGNORE TRANSPARENCY in order to avoid accidentally importing transparent meshes (which will
            // affect the performance), instead force the developer to carefully think and enable transparency
            // (in the editor) for meshes that actually need it.
            // if (material.alphaMode == "MASK") {
            //     Logger::get().warn(
            //         "found material with transparency enabled, transparent materials have "
            //         "very serious impact on the performance so you might want to avoid using them");
            //     meshMaterial.setEnableTransparency(true);
            //     meshMaterial.setOpacity(
            //         1.0F - static_cast<float>(std::clamp(material.alphaCutoff, 0.0, 1.0)));
            // }

            // Process base color.
            meshMaterial.setDiffuseColor(glm::vec3(
                material.pbrMetallicRoughness.baseColorFactor[0],
                material.pbrMetallicRoughness.baseColorFactor[1],
                material.pbrMetallicRoughness.baseColorFactor[2]));

            // Process diffuse texture.
            const auto iDiffuseTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
            if (iDiffuseTextureIndex >= 0) {
                auto& diffuseTexture = model.textures[static_cast<size_t>(iDiffuseTextureIndex)];
                if (diffuseTexture.source >= 0) {
                    // Get image.
                    auto& diffuseImage = model.images[static_cast<size_t>(diffuseTexture.source)];

                    if (!std::filesystem::exists(pathToTexturesDir)) {
                        std::filesystem::create_directory(pathToTexturesDir);
                    }

                    // Construct path to imported texture directory.
                    const auto pathDiffuseTextureRelativeRes =
                        pathToTexturesDir / std::format(
                                                "{}_{}.{}",
                                                sDiffuseTextureName,
                                                std::to_string(iPrimitive),
                                                sImportedImageExtension);

                    const auto sTexturePathRelativeRes =
                        std::filesystem::relative(
                            pathDiffuseTextureRelativeRes,
                            ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT))
                            .string();

                    // Write image to disk.
                    if (!writeGltfTextureToDisk(diffuseImage, sTexturePathRelativeRes)) [[unlikely]] {
                        return Error(std::format(
                            "failed to write GLTF image to path \"{}\"", sTexturePathRelativeRes));
                    }

                    // Specify texture path.
                    meshMaterial.setPathToDiffuseTexture(sTexturePathRelativeRes);
                }
            }
        }

        // Add this new mesh node to results.
        vMeshNodes.push_back(std::move(pMeshNode));
    }

    return vMeshNodes;
}

inline std::optional<Error> processGltfNode(
    const tinygltf::Node& node,
    const tinygltf::Model& model,
    const std::filesystem::path& pathToOutputFile,
    const std::string& sPathToOutputDirRelativeRes,
    Node* pParentNode,
    const std::function<void(std::string_view)>& onProgress,
    size_t& iGltfNodeProcessedCount,
    const size_t iTotalGltfNodesToProcess) {
    // Prepare a node that will store this GLTF node.
    Node* pThisNode = pParentNode;

    // See if this node stores a mesh.
    if ((node.mesh >= 0) && (node.mesh < static_cast<int>(model.meshes.size()))) {
        // Process mesh.
        auto result = processGltfMesh(
            model,
            model.meshes[static_cast<size_t>(node.mesh)],
            pathToOutputFile,
            sPathToOutputDirRelativeRes,
            onProgress,
            iGltfNodeProcessedCount,
            iTotalGltfNodesToProcess);
        if (std::holds_alternative<Error>(result)) [[unlikely]] {
            auto error = std::get<Error>(std::move(result));
            error.addCurrentLocationToErrorStack();
            return error;
        }
        auto vMeshNodes = std::get<std::vector<std::unique_ptr<MeshNode>>>(std::move(result));

        // Attach new nodes to parent.
        for (auto& pMeshNode : vMeshNodes) {
            // Attach to parent node.
            pParentNode->addChildNode(
                std::move(pMeshNode),
                Node::AttachmentRule::KEEP_RELATIVE,
                Node::AttachmentRule::KEEP_RELATIVE, // don't change relative coordinates
                Node::AttachmentRule::KEEP_RELATIVE);

            // Mark this node as parent for child GLTF nodes.
            pThisNode = pMeshNode.get();
        }
    }

    // Mark node as processed.
    iGltfNodeProcessedCount += 1;
    onProgress(std::format("processing GLTF node {}/{}", iGltfNodeProcessedCount, iTotalGltfNodesToProcess));

    // Process child nodes.
    for (const auto& iNode : node.children) {
        // Process child node.
        auto optionalError = processGltfNode(
            model.nodes[static_cast<size_t>(iNode)],
            model,
            pathToOutputFile,
            sPathToOutputDirRelativeRes,
            pThisNode,
            onProgress,
            iGltfNodeProcessedCount,
            iTotalGltfNodesToProcess);
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            return optionalError;
        }
    }

    return {};
}

std::optional<Error> GltfImporter::importFileAsNodeTree(
    const std::filesystem::path& pathToFile,
    const std::string& sPathToOutputDirRelativeRes,
    const std::string& sOutputDirectoryName,
    const std::function<void(std::string_view)>& onProgress) {
    // Make sure the file has ".GLTF" or ".GLB" extension.
    if (pathToFile.extension() != ".GLTF" && pathToFile.extension() != ".gltf" &&
        pathToFile.extension() != ".GLB" && pathToFile.extension() != ".glb") [[unlikely]] {
        return Error(std::format(
            "only GLTF/GLB file extension is supported for mesh import, the path \"{}\" points to a "
            "non-GLTF file",
            pathToFile.string()));
    }

    // Make sure the specified path to the file exists.
    if (!std::filesystem::exists(pathToFile)) [[unlikely]] {
        return Error(std::format("the specified path \"{}\" does not exists", pathToFile.string()));
    }

    // Construct an absolute path to the output directory.
    const auto pathToOutputDirectoryParent =
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ROOT) / sPathToOutputDirRelativeRes;

    // Make sure the output directory exists.
    if (!std::filesystem::exists(pathToOutputDirectoryParent)) [[unlikely]] {
        return Error(std::format(
            "expected the specified path output directory \"{}\" to exist",
            pathToOutputDirectoryParent.string()));
    }

    // Make sure the specified directory name is not empty.
    if (sOutputDirectoryName.empty()) [[unlikely]] {
        return Error("expected the specified directory name to not be empty");
    }

    // Make sure the specified directory name is not very long
    // to avoid creating long paths which might be an issue under Windows.
    static constexpr size_t iMaxOutputDirectoryNameLength = 16;
    if (sOutputDirectoryName.size() > iMaxOutputDirectoryNameLength) [[unlikely]] {
        return Error(std::format(
            "the specified name \"{}\" is too long (only {} characters allowed)",
            sOutputDirectoryName,
            iMaxOutputDirectoryNameLength));
    }

    // Make sure the specified directory name is valid (A-z, 0-9).
    constexpr std::string_view sValidCharacters =
        "01234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";
    for (const auto& character : sOutputDirectoryName) {
        if (sValidCharacters.find(character) == std::string::npos) [[unlikely]] {
            return Error(std::format(
                "character \"{}\" in the name \"{}\" is forbidden and cannon be used",
                character,
                sOutputDirectoryName));
        }
    }

    // Make sure the specified resulting directory does not exists yet.
    const auto pathToOutputDirectory = pathToOutputDirectoryParent / sOutputDirectoryName;
    const auto pathToOutputFile = pathToOutputDirectoryParent / sOutputDirectoryName /
                                  (sOutputDirectoryName + ConfigManager::getConfigFormatExtension());
    if (std::filesystem::exists(pathToOutputDirectory)) [[unlikely]] {
        return Error(std::format(
            "expected the resulting directory \"{}\" to not exist", pathToOutputDirectory.string()));
    }

    // Create resulting directory.
    std::filesystem::create_directory(pathToOutputDirectory);

    // See if we have a binary GTLF file or not.
    bool bIsGlb = false;
    if (pathToFile.extension() == ".GLB" || pathToFile.extension() == ".glb") {
        bIsGlb = true;
    }

    // Prepare variables for storing results.
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string sError;
    std::string sWarning;
    bool bIsSuccess = false;

    // Don't make all images to be in RGBA format.
    loader.SetPreserveImageChannels(true);

    // Mark progress.
    onProgress("parsing file");

    // Load data from file.
    if (bIsGlb) {
        bIsSuccess = loader.LoadBinaryFromFile(&model, &sError, &sWarning, pathToFile.string());
    } else {
        bIsSuccess = loader.LoadASCIIFromFile(&model, &sError, &sWarning, pathToFile.string());
    }

    // See if there were any warnings/errors.
    if (!sWarning.empty()) {
        // Treat warnings as errors.
        return Error(std::format("there was an error during the import process: {}", sWarning));
    }
    if (!sError.empty()) {
        return Error(std::format("there was an error during the import process: {}", sError));
    }
    if (!bIsSuccess) {
        return Error("there was an error during the import process but no error message was received");
    }

    // Get default scene.
    const auto& scene = model.scenes[static_cast<size_t>(model.defaultScene)];

    // Create a scene root node to hold all GLTF nodes of the scene.
    auto pSceneRootNode = std::make_unique<Node>("Scene Root");

    // Prepare variable for processed nodes.
    size_t iTotalNodeProcessedCount = 0;

    // Now process GLTF nodes.
    for (const auto& iNode : scene.nodes) {
        // Make sure this node index is valid.
        if (iNode < 0) [[unlikely]] {
            return Error(std::format("found a negative node index of {} in default scene", iNode));
        }
        if (iNode >= static_cast<int>(model.nodes.size())) [[unlikely]] {
            return Error(std::format(
                "found an out of bounds node index of {} while model nodes only has {} entries",
                iNode,
                model.nodes.size()));
        }

        // Process node.
        auto optionalError = processGltfNode(
            model.nodes[static_cast<size_t>(iNode)],
            model,
            pathToOutputFile,
            std::format("{}/{}", sPathToOutputDirRelativeRes, sOutputDirectoryName),
            pSceneRootNode.get(),
            onProgress,
            iTotalNodeProcessedCount,
            scene.nodes.size());
        if (optionalError.has_value()) [[unlikely]] {
            optionalError->addCurrentLocationToErrorStack();
            return optionalError;
        }
    }

    // Mark progress.
    onProgress("serializing resulting node tree");

    if (pSceneRootNode->mtxChildNodes.second.size() == 1 &&
        pSceneRootNode->mtxChildNodes.second[0]->getChildNodes().second.empty()) {
        // Only 1 node was imported, remove our scene root node since there's no need for it.
        auto pNewRoot = std::move(pSceneRootNode->mtxChildNodes.second[0]);
        pSceneRootNode->mtxChildNodes.second.clear();

        pSceneRootNode = std::move(pNewRoot);
    }

    // Serialize scene node tree.
    auto optionalError = pSceneRootNode->serializeNodeTree(pathToOutputFile, false);
    if (optionalError.has_value()) [[unlikely]] {
        optionalError->addCurrentLocationToErrorStack();
        return optionalError;
    }

    // Mark progress.
    onProgress("finished");

    return {};
}
