//#include "SceneConvert.h"
//
//#include <iostream>
//
//#include <Core/Logger.h>
//
//#include <meshoptimizer.h>
//
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_RESIZE_IMPLEMENTATION
//#include <stb_image.h>
//#include <stb_image_resize.h>
//#include <stb_image_write.h>
//
//namespace fs = std::filesystem;
//
//namespace Suoh
//{
//
//namespace SceneConvert
//{
//
//namespace
//{
//
//glm::mat4 toMat4(const aiMatrix4x4& from)
//{
//    glm::mat4 to;
//    to[0][0] = (float)from.a1;
//    to[0][1] = (float)from.b1;
//    to[0][2] = (float)from.c1;
//    to[0][3] = (float)from.d1;
//    to[1][0] = (float)from.a2;
//    to[1][1] = (float)from.b2;
//    to[1][2] = (float)from.c2;
//    to[1][3] = (float)from.d2;
//    to[2][0] = (float)from.a3;
//    to[2][1] = (float)from.b3;
//    to[2][2] = (float)from.c3;
//    to[2][3] = (float)from.d3;
//    to[3][0] = (float)from.a4;
//    to[3][1] = (float)from.b4;
//    to[3][2] = (float)from.c4;
//    to[3][3] = (float)from.d4;
//    return to;
//}
//
//void traverse(const aiScene* sourceScene, Scene::SceneContext& scene, aiNode* N, int parent, int ofs)
//{
//    int newNode = addNode(scene, parent, ofs);
//
//    if (N->mName.C_Str())
//    {
//        // makePrefix(ofs);
//        // printf("Node[%d].name = %s\n", newNode, N->mName.C_Str());
//
//        u32 stringID = (u32)scene.names.size();
//        scene.names.push_back(std::string(N->mName.C_Str()));
//        scene.namesMap[newNode] = stringID;
//    }
//
//    for (size_t i = 0; i < N->mNumMeshes; i++)
//    {
//        int newSubNode = addNode(scene, newNode, ofs + 1);
//
//        u32 stringID = (u32)scene.names.size();
//        scene.names.push_back(std::string(N->mName.C_Str()) + "_Mesh_" + std::to_string(i));
//        scene.namesMap[newSubNode] = stringID;
//
//        int mesh = (int)N->mMeshes[i];
//        scene.meshesMap[newSubNode] = mesh;
//        scene.materialsMap[newSubNode] = sourceScene->mMeshes[mesh]->mMaterialIndex;
//
//        // makePrefix(ofs);
//        // printf("Node[%d].SubNode[%d].mesh     = %d\n", newNode, newSubNode, (int)mesh);
//        // makePrefix(ofs);
//        // printf("Node[%d].SubNode[%d].material = %d\n", newNode, newSubNode, sourceScene->mMeshes[mesh]->mMaterialIndex);
//
//        scene.globalTransforms[newSubNode] = glm::mat4(1.0f);
//        scene.localTransforms[newSubNode] = glm::mat4(1.0f);
//    }
//
//    scene.globalTransforms[newNode] = glm::mat4(1.0f);
//    scene.localTransforms[newNode] = toMat4(N->mTransformation);
//
//    if (N->mParent != nullptr)
//    {
//        // makePrefix(ofs);
//        // printf("\tNode[%d].parent         = %s\n", newNode, N->mParent->mName.C_Str());
//        // makePrefix(ofs);
//        // printf("\tNode[%d].localTransform = ", newNode);
//        // printMat4(N->mTransformation);
//        // printf("\n");
//    }
//
//    for (unsigned int n = 0; n < N->mNumChildren; n++)
//        traverse(sourceScene, scene, N->mChildren[n], newNode, ofs + 1);
//}
//
//void dumpMaterial(const std::vector<std::string>& files, const MaterialDescription& d)
//{
//    printf("files: %d\n", (int)files.size());
//    printf("maps: %u/%u/%u/%u/%u\n", (u32)d.albedoMap, (u32)d.ambientOcclusionMap, (u32)d.emissiveMap, (u32)d.opacityMap,
//           (u32)d.metallicRoughnessMap);
//    printf(" albedo:    %s\n", (d.albedoMap < 0xFFFF) ? files[d.albedoMap].c_str() : "");
//    printf(" occlusion: %s\n", (d.ambientOcclusionMap < 0xFFFF) ? files[d.ambientOcclusionMap].c_str() : "");
//    printf(" emission:  %s\n", (d.emissiveMap < 0xFFFF) ? files[d.emissiveMap].c_str() : "");
//    printf(" opacity:   %s\n", (d.opacityMap < 0xFFFF) ? files[d.opacityMap].c_str() : "");
//    printf(" MeR:       %s\n", (d.metallicRoughnessMap < 0xFFFF) ? files[d.metallicRoughnessMap].c_str() : "");
//    printf(" Normal:    %s\n", (d.normalMap < 0xFFFF) ? files[d.normalMap].c_str() : "");
//}
//
//std::string replaceAll(const std::string& str, const std::string& oldSubStr, const std::string& newSubStr)
//{
//    std::string result = str;
//
//    for (size_t p = result.find(oldSubStr); p != std::string::npos; p = result.find(oldSubStr))
//        result.replace(p, oldSubStr.length(), newSubStr);
//
//    return result;
//}
//
///* Convert 8-bit ASCII string to upper case */
//std::string lowercaseString(const std::string& s)
//{
//    std::string out(s.length(), ' ');
//    std::transform(s.begin(), s.end(), out.begin(), tolower);
//    return out;
//}
//
///* find a file in directory which "almost" coincides with the origFile (their lowercase versions coincide) */
//std::string findSubstitute(const std::string& origFile)
//{
//    // Make absolute path
//    auto apath = fs::absolute(fs::path(origFile));
//    // Absolute lowercase filename [we compare with it]
//    auto afile = lowercaseString(apath.filename().string());
//    // Directory where in which the file should be
//    auto dir = fs::path(origFile).remove_filename();
//
//    // Iterate each file non-recursively and compare lowercase absolute path with 'afile'
//    for (auto& p : fs::directory_iterator(dir))
//        if (afile == lowercaseString(p.path().filename().string()))
//            return p.path().string();
//
//    return std::string{};
//}
//
//std::string fixTextureFile(const std::string& file)
//{
//    // TODO: check the findSubstitute() function
//    return fs::exists(file) ? file : findSubstitute(file);
//}
//
//std::string convertTexture(const std::string& file, const std::string& basePath, std::unordered_map<std::string, u32>& opacityMapIndices,
//                           const std::vector<std::string>& opacityMaps)
//{
//    const int maxNewWidth = 512;
//    const int maxNewHeight = 512;
//
//    const auto srcFile = replaceAll(basePath + file, "\\", "/");
//    const auto newFile = std::string("../../../../../Resources/bistro/OutTextures2/")
//                         + lowercaseString(replaceAll(replaceAll(srcFile, "..", "__"), "/", "__") + std::string("__rescaled"))
//                         + std::string(".png");
//
//    // load this image
//    int texWidth, texHeight, texChannels;
//    stbi_uc* pixels = stbi_load(fixTextureFile(srcFile).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//    uint8_t* src = pixels;
//    texChannels = STBI_rgb_alpha;
//
//    std::vector<uint8_t> tmpImage(maxNewWidth * maxNewHeight * 4);
//
//    if (!src)
//    {
//        printf("Failed to load [%s] texture\n", srcFile.c_str());
//        texWidth = maxNewWidth;
//        texHeight = maxNewHeight;
//        texChannels = STBI_rgb_alpha;
//        src = tmpImage.data();
//    }
//    else
//    {
//        printf("Loaded [%s] %dx%d texture with %d channels\n", srcFile.c_str(), texWidth, texHeight, texChannels);
//    }
//
//    if (opacityMapIndices.count(file) > 0)
//    {
//        const auto opacityMapFile = replaceAll(basePath + opacityMaps[opacityMapIndices[file]], "\\", "/");
//        int opacityWidth, opacityHeight;
//        stbi_uc* opacityPixels = stbi_load(fixTextureFile(opacityMapFile).c_str(), &opacityWidth, &opacityHeight, nullptr, 1);
//
//        if (!opacityPixels)
//        {
//            printf("Failed to load opacity mask [%s]\n", opacityMapFile.c_str());
//        }
//
//        std::cout << "OPACITY LOAD: " << opacityMapFile << std::endl;
//
//        assert(opacityPixels);
//        assert(texWidth == opacityWidth);
//        assert(texHeight == opacityHeight);
//
//        // store the opacity mask in the alpha component of this image
//        if (opacityPixels)
//            for (int y = 0; y != opacityHeight; y++)
//                for (int x = 0; x != opacityWidth; x++)
//                    src[(y * opacityWidth + x) * texChannels + 3] = opacityPixels[y * opacityWidth + x];
//
//        stbi_image_free(opacityPixels);
//    }
//
//    const u32 imgSize = texWidth * texHeight * texChannels;
//    std::vector<uint8_t> mipData(imgSize);
//    uint8_t* dst = mipData.data();
//
//    const int newW = std::min(texWidth, maxNewWidth);
//    const int newH = std::min(texHeight, maxNewHeight);
//
//    stbir_resize_uint8(src, texWidth, texHeight, 0, dst, newW, newH, 0, texChannels);
//
//    int write_res = stbi_write_png(newFile.c_str(), newW, newH, texChannels, dst, 0);
//
//    if (!write_res)
//    {
//        std::cout << "[FAILED WRITE] " << newFile.c_str() << std::endl;
//    }
//    else
//    {
//        std::cout << "[WRITTEN TO] " << newFile.c_str() << std::endl;
//    }
//
//    if (pixels)
//        stbi_image_free(pixels);
//
//    return newFile;
//}
//
//void convertAndDownscaleAllTextures(const std::vector<MaterialDescription>& materials, const std::string& basePath,
//                                    std::vector<std::string>& files, std::vector<std::string>& opacityMaps)
//{
//    std::unordered_map<std::string, u32> opacityMapIndices(files.size());
//
//    for (const auto& m : materials)
//        if (m.opacityMap != 0xFFFFFFFF && m.albedoMap != 0xFFFFFFFF)
//            opacityMapIndices[files[m.albedoMap]] = (u32)m.opacityMap;
//
//    auto converter = [&](const std::string& s) -> std::string { return convertTexture(s, basePath, opacityMapIndices, opacityMaps); };
//
//    std::transform(std::execution::par, std::begin(files), std::end(files), std::begin(files), converter);
//}
//
//} // namespace
//
//// Number of floats for a vertex point.
//static constexpr auto NUM_VERTEX_ELEMENTS = 3 + 3 + 2;
//
//MaterialDescription convertAIMaterialToMaterialDescription(const aiMaterial* M, std::vector<std::string>& files,
//                                                           std::vector<std::string>& opacityMaps)
//{
//    MaterialDescription matDescription;
//
//    aiColor4D Color;
//
//    if (aiGetMaterialColor(M, AI_MATKEY_COLOR_AMBIENT, &Color) == AI_SUCCESS)
//    {
//        matDescription.emissiveColor = {Color.r, Color.g, Color.b, Color.a};
//        if (matDescription.emissiveColor.w > 1.0f)
//            matDescription.emissiveColor.w = 1.0f;
//    }
//    if (aiGetMaterialColor(M, AI_MATKEY_COLOR_DIFFUSE, &Color) == AI_SUCCESS)
//    {
//        matDescription.albedoColor = {Color.r, Color.g, Color.b, Color.a};
//        if (matDescription.albedoColor.w > 1.0f)
//            matDescription.albedoColor.w = 1.0f;
//    }
//    if (aiGetMaterialColor(M, AI_MATKEY_COLOR_EMISSIVE, &Color) == AI_SUCCESS)
//    {
//        matDescription.emissiveColor.x += Color.r;
//        matDescription.emissiveColor.y += Color.g;
//        matDescription.emissiveColor.z += Color.b;
//        matDescription.emissiveColor.w += Color.a;
//        if (matDescription.emissiveColor.w > 1.0f)
//            matDescription.albedoColor.w = 1.0f;
//    }
//
//    const float opaquenessThreshold = 0.05f;
//    float Opacity = 1.0f;
//
//    if (aiGetMaterialFloat(M, AI_MATKEY_OPACITY, &Opacity) == AI_SUCCESS)
//    {
//        matDescription.transparencyFactor = glm::clamp(1.0f - Opacity, 0.0f, 1.0f);
//        if (matDescription.transparencyFactor >= 1.0f - opaquenessThreshold)
//            matDescription.transparencyFactor = 0.0f;
//    }
//
//    if (aiGetMaterialColor(M, AI_MATKEY_COLOR_TRANSPARENT, &Color) == AI_SUCCESS)
//    {
//        const float Opacity = std::max(std::max(Color.r, Color.g), Color.b);
//        matDescription.transparencyFactor = glm::clamp(Opacity, 0.0f, 1.0f);
//        if (matDescription.transparencyFactor >= 1.0f - opaquenessThreshold)
//            matDescription.transparencyFactor = 0.0f;
//        matDescription.alphaTest = 0.5f;
//    }
//
//    float tmp = 1.0f;
//    if (aiGetMaterialFloat(M, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &tmp) == AI_SUCCESS)
//        matDescription.metallicFactor = tmp;
//
//    if (aiGetMaterialFloat(M, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &tmp) == AI_SUCCESS)
//        matDescription.roughness = {tmp, tmp, tmp, tmp};
//
//    aiString Path;
//    aiTextureMapping Mapping;
//    unsigned int UVIndex = 0;
//    float Blend = 1.0f;
//    aiTextureOp TextureOp = aiTextureOp_Add;
//    aiTextureMapMode TextureMapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
//    unsigned int TextureFlags = 0;
//
//    if (aiGetMaterialTexture(M, aiTextureType_EMISSIVE, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags)
//        == AI_SUCCESS)
//    {
//        matDescription.emissiveMap = addUnique(files, Path.C_Str());
//    }
//
//    if (aiGetMaterialTexture(M, aiTextureType_DIFFUSE, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags)
//        == AI_SUCCESS)
//    {
//        matDescription.albedoMap = addUnique(files, Path.C_Str());
//        const std::string albedoMap = std::string(Path.C_Str());
//        if (albedoMap.find("grey_30") != albedoMap.npos)
//            matDescription.flags |= MaterialFlags_Transparent;
//    }
//
//    // first try tangent space normal map
//    if (aiGetMaterialTexture(M, aiTextureType_NORMALS, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags)
//        == AI_SUCCESS)
//    {
//        matDescription.normalMap = addUnique(files, Path.C_Str());
//    }
//    // then height map
//    if (matDescription.normalMap == 0xFFFFFFFF)
//        if (aiGetMaterialTexture(M, aiTextureType_HEIGHT, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags)
//            == AI_SUCCESS)
//            matDescription.normalMap = addUnique(files, Path.C_Str());
//
//    if (aiGetMaterialTexture(M, aiTextureType_OPACITY, 0, &Path, &Mapping, &UVIndex, &Blend, &TextureOp, TextureMapMode, &TextureFlags)
//        == AI_SUCCESS)
//    {
//        matDescription.opacityMap = addUnique(opacityMaps, Path.C_Str());
//        matDescription.alphaTest = 0.5f;
//    }
//
//    // patch materials
//    aiString Name;
//    std::string materialName;
//    if (aiGetMaterialString(M, AI_MATKEY_NAME, &Name) == AI_SUCCESS)
//    {
//        materialName = Name.C_Str();
//    }
//
//    // apply heuristics for specific materials
//    if ((materialName.find("Glass") != std::string::npos) || (materialName.find("Vespa_Headlight") != std::string::npos))
//    {
//        matDescription.alphaTest = 0.75f;
//        matDescription.transparencyFactor = 0.1f;
//        matDescription.flags |= MaterialFlags_Transparent;
//    }
//    else if (materialName.find("Bottle") != std::string::npos)
//    {
//        matDescription.alphaTest = 0.54f;
//        matDescription.transparencyFactor = 0.4f;
//        matDescription.flags |= MaterialFlags_Transparent;
//    }
//    else if (materialName.find("Metal") != std::string::npos)
//    {
//        matDescription.metallicFactor = 1.0f;
//        matDescription.roughness = gpuvec4(0.1f, 0.1f, 0.0f, 0.0f);
//    }
//
//    return matDescription;
//}
//
//void processLods(std::vector<u32>& indices, std::vector<float>& vertices, std::vector<std::vector<u32>>& outLods)
//{
//    size_t verticesCountIn = vertices.size() / 2;
//    size_t targetIndicesCount = indices.size();
//
//    u8 LOD = 1;
//
//    LOG_INFO("processLods: indices count at the start ", indices.size());
//
//    outLods.push_back(indices);
//
//    while (targetIndicesCount > 1024 && LOD < 8)
//    {
//        targetIndicesCount = indices.size() / 2;
//
//        bool sloppy = false;
//
//        size_t numOptIndices = meshopt_simplify(indices.data(), indices.data(), (u32)indices.size(), vertices.data(), verticesCountIn,
//                                                sizeof(float) * 3, targetIndicesCount, 0.02f);
//
//        // cannot simplify further
//        if (static_cast<size_t>(numOptIndices * 1.1f) > indices.size())
//        {
//            if (LOD > 1)
//            {
//                // try harder
//                numOptIndices = meshopt_simplifySloppy(indices.data(), indices.data(), indices.size(), vertices.data(), verticesCountIn,
//                                                       sizeof(float) * 3, targetIndicesCount, 0.02f, nullptr);
//                sloppy = true;
//                if (numOptIndices == indices.size())
//                    break;
//            }
//            else
//                break;
//        }
//
//        indices.resize(numOptIndices);
//
//        meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), verticesCountIn);
//
//        LOG_INFO("processLods: count of indices at LOD ", LOD, ": ", numOptIndices, ", sloppy: ", sloppy);
//
//        LOD++;
//
//        outLods.push_back(indices);
//    }
//};
//
//Mesh convertAIMesh(const aiMesh* aimesh, const SceneConfig& config, MeshData& meshData, u32& indexOffset, u32& vertexOffset)
//{
//    const bool hasTexCoords = aimesh->HasTextureCoords(0);
//    const u32 streamElementSize = static_cast<u32>(NUM_VERTEX_ELEMENTS * sizeof(float));
//
//    // Original data for LOD calculation
//    std::vector<float> srcVertices;
//    std::vector<u32> srcIndices;
//
//    std::vector<std::vector<u32>> outLods;
//
//    auto& vertices = meshData.vertexData;
//
//    for (auto i = 0; i != aimesh->mNumVertices; i++)
//    {
//        // vertices
//        const aiVector3D v = aimesh->mVertices[i];
//
//        // normals
//        const aiVector3D n = aimesh->mNormals[i];
//
//        // texcoords
//        const aiVector3D t = hasTexCoords ? aimesh->mTextureCoords[0][i] : aiVector3D();
//
//        if (config.calculateLODs)
//        {
//            srcVertices.push_back(v.x);
//            srcVertices.push_back(v.y);
//            srcVertices.push_back(v.z);
//        }
//
//        vertices.push_back(v.x * config.scale);
//        vertices.push_back(v.y * config.scale);
//        vertices.push_back(v.z * config.scale);
//
//        vertices.push_back(t.x);
//        vertices.push_back(1.0f - t.y);
//
//        vertices.push_back(n.x);
//        vertices.push_back(n.y);
//        vertices.push_back(n.z);
//    }
//
//    Mesh result = {
//        .streamCount = 1,
//        .indexOffset = indexOffset,
//        .vertexOffset = vertexOffset,
//        .vertexCount = aimesh->mNumVertices,
//        .streamOffset = {vertexOffset * streamElementSize},
//        .streamElementSize = {streamElementSize},
//    };
//
//    for (auto i = 0; i != aimesh->mNumFaces; i++)
//    {
//        if (aimesh->mFaces[i].mNumIndices != 3)
//            continue;
//        for (auto j = 0; j != aimesh->mFaces[i].mNumIndices; j++)
//            srcIndices.push_back(aimesh->mFaces[i].mIndices[j]);
//    }
//
//    if (!config.calculateLODs)
//        outLods.push_back(srcIndices);
//    else
//        processLods(srcIndices, srcVertices, outLods);
//
//    // LOG_INFO("Number of vertices: ", aimesh->mNumVertices);
//    // LOG_INFO("Calculated LOD count: ", outLods.size());
//
//    u32 numIndices = 0;
//    for (auto l = 0; l < outLods.size(); l++)
//    {
//        for (auto i = 0; i < outLods[l].size(); i++)
//            meshData.indexData.push_back(outLods[l][i]);
//
//        // LOG_INFO("Number of indices for LOD ", l, ": ", outLods[l].size());
//
//        result.lodOffset[l] = numIndices;
//        numIndices += (int)outLods[l].size();
//    }
//
//    result.lodOffset[outLods.size()] = numIndices;
//    result.lodCount = (u32)outLods.size();
//
//    indexOffset += numIndices;
//    vertexOffset += aimesh->mNumVertices;
//
//    return result;
//}
//
//void processScene(const SceneConfig& cfg)
//{
//    // clear mesh data from previous scene
//    // g_MeshData.meshes_.clear();
//    // g_MeshData.boxes_.clear();
//    // g_MeshData.indexData_.clear();
//    // g_MeshData.vertexData_.clear();
//
//    // g_indexOffset = 0;
//    // g_vertexOffset = 0;
//
//    MeshData meshData;
//    u32 indexOffset = 0;
//    u32 vertexOffset = 0;
//
//    // extract base model path
//    const std::size_t pathSeparator = cfg.fileName.find_last_of("/\\");
//    const std::string basePath = (pathSeparator != std::string::npos) ? cfg.fileName.substr(0, pathSeparator + 1) : std::string();
//
//    const unsigned int flags = 0 | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals
//                               | aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes | aiProcess_ImproveCacheLocality
//                               | aiProcess_RemoveRedundantMaterials | aiProcess_FindDegenerates | aiProcess_FindInvalidData
//                               | aiProcess_GenUVCoords;
//
//    printf("Loading scene from '%s'...\n", cfg.fileName.c_str());
//
//    Assimp::Importer import;
//    const aiScene* scene = import.ReadFile(cfg.fileName.c_str(), flags);
//
//    if (!scene || !scene->HasMeshes())
//    {
//        // printf("Unable to load '%s'\n", fs::absolute(cfg.fileName));
//        std::cout << "Unable to load " << fs::absolute(cfg.fileName) << std::endl;
//        exit(EXIT_FAILURE);
//    }
//
//    // 1. Mesh conversion as in Chapter 5
//    meshData.meshes.reserve(scene->mNumMeshes);
//    meshData.boxes.reserve(scene->mNumMeshes);
//
//    for (unsigned int i = 0; i != scene->mNumMeshes; i++)
//    {
//        printf("\nConverting meshes %u/%u...", i + 1, scene->mNumMeshes);
//        Mesh mesh = convertAIMesh(scene->mMeshes[i], cfg, meshData, indexOffset, vertexOffset);
//        meshData.meshes.push_back(mesh);
//    }
//
//    recalculateBoundingBoxes(meshData);
//
//    saveMeshData(cfg.outputMesh.c_str(), meshData);
//
//    Scene::SceneContext ourScene;
//
//    // 2. Material conversion
//    std::vector<MaterialDescription> materials;
//    std::vector<std::string>& materialNames = ourScene.materialNames;
//
//    std::vector<std::string> files;
//    std::vector<std::string> opacityMaps;
//
//    for (unsigned int m = 0; m < scene->mNumMaterials; m++)
//    {
//        aiMaterial* mm = scene->mMaterials[m];
//
//        printf("Material [%s] %u\n", mm->GetName().C_Str(), m);
//        materialNames.push_back(std::string(mm->GetName().C_Str()));
//
//        MaterialDescription D = convertAIMaterialToMaterialDescription(mm, files, opacityMaps);
//        materials.push_back(D);
//        // dumpMaterial(files, D);
//    }
//
//    // 3. Texture processing, rescaling and packing
//    convertAndDownscaleAllTextures(materials, basePath, files, opacityMaps);
//
//    for (auto& file : files)
//        std::cout << file << std::endl;
//
//    saveMaterials(cfg.outputMaterials, materials, files);
//
//    // 4. Scene hierarchy conversion
//    traverse(scene, ourScene, scene->mRootNode, -1, 0);
//
//    saveScene(cfg.outputScene, ourScene);
//}
//
//} // namespace SceneConvert
//
//} // namespace Suoh
