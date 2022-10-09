#include "MeshConvert.h"

#include <Core/Logger.h>

#include <assimp/Importer.hpp>
#include <meshoptimizer.h>

namespace Suoh
{

namespace MeshConvert
{

// 3 for vertex + 3 for normal + 2 for texcoords
static constexpr auto g_numElementsToStore = 3 + 3 + 2;

float g_meshScale = 0.01f;
bool g_calculateLODs = false;

void processLods(std::vector<u32>& indices, std::vector<float>& vertices, std::vector<std::vector<u32>>& outLods)
{
    size_t verticesCountIn = vertices.size() / 2;
    size_t targetIndicesCount = indices.size();

    u8 LOD = 1;

    outLods.push_back(indices);

    while (targetIndicesCount > 1024 && LOD < 8)
    {
        targetIndicesCount = indices.size() / 2;

        size_t numOptIndices = meshopt_simplify(
            indices.data(),
            indices.data(), (u32)indices.size(),
            vertices.data(), verticesCountIn,
            sizeof(float) * 3,
            targetIndicesCount, 0.02f);

        // cannot simplify further
        if (static_cast<size_t>(numOptIndices * 1.1f) > indices.size())
        {
            if (LOD > 1)
            {
                // try harder
                numOptIndices = meshopt_simplifySloppy(
                    indices.data(),
                    indices.data(), indices.size(),
                    vertices.data(), verticesCountIn,
                    sizeof(float) * 3,
                    targetIndicesCount, 0.02f);
                if (numOptIndices == indices.size())
                    break;
            }
            else
                break;
        }

        indices.resize(numOptIndices);

        meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), verticesCountIn);

        LOD++;

        outLods.push_back(indices);
    }
}

Mesh convertAIMesh(const aiMesh* aimesh, MeshData& meshData, u32& indexOffset, u32& vertexOffset)
{
    const bool hasTexCoords = aimesh->HasTextureCoords(0);
    const u32 streamElementSize = static_cast<u32>(g_numElementsToStore * sizeof(float));

    // Original data for LOD calculation
    std::vector<float> srcVertices;
    std::vector<u32> srcIndices;

    std::vector<std::vector<u32>> outLods;

    auto& vertices = meshData.vertexData;

    for (auto i = 0; i != aimesh->mNumVertices; i++)
    {
        // vertices
        const aiVector3D v = aimesh->mVertices[i];

        // normals
        const aiVector3D n = aimesh->mNormals[i];

        // texcoords
        const aiVector3D t = hasTexCoords ? aimesh->mTextureCoords[0][i] : aiVector3D();

        if (g_calculateLODs)
        {
            srcVertices.push_back(v.x);
            srcVertices.push_back(v.y);
            srcVertices.push_back(v.z);
        }

        vertices.push_back(v.x * g_meshScale);
        vertices.push_back(v.y * g_meshScale);
        vertices.push_back(v.z * g_meshScale);

        vertices.push_back(t.x);
        vertices.push_back(1.0f - t.y);

        vertices.push_back(n.x);
        vertices.push_back(n.y);
        vertices.push_back(n.z);
    }

    Mesh result = {
        .streamCount = 1,
        .indexOffset = indexOffset,
        .vertexOffset = vertexOffset,
        .vertexCount = aimesh->mNumVertices,
        .streamOffset = {vertexOffset * streamElementSize},
        .streamElementSize = {streamElementSize},
    };

    for (auto i = 0; i != aimesh->mNumFaces; i++)
    {
        if (aimesh->mFaces[i].mNumIndices != 3)
            continue;
        for (auto j = 0; j != aimesh->mFaces[i].mNumIndices; j++)
            srcIndices.push_back(aimesh->mFaces[i].mIndices[j]);
    }

    if (!g_calculateLODs)
        outLods.push_back(srcIndices);
    else
        processLods(srcIndices, srcVertices, outLods);

    // LOG_INFO("Number of vertices: ", aimesh->mNumVertices);
    // LOG_INFO("Calculated LOD count: ", outLods.size());

    u32 numIndices = 0;
    for (auto l = 0; l < outLods.size(); l++)
    {
        for (auto i = 0; i < outLods[l].size(); i++)
            meshData.indexData.push_back(outLods[l][i]);

        // LOG_INFO("Number of indices for LOD ", l, ": ", outLods[l].size());

        result.lodOffset[l] = numIndices;
        numIndices += (int)outLods[l].size();
    }

    result.lodOffset[outLods.size()] = numIndices;
    result.lodCount = (u32)outLods.size();

    indexOffset += numIndices;
    vertexOffset += aimesh->mNumVertices;

    return result;
}

bool loadFileToMeshData(MeshData& meshData, const std::string& filePath)
{
    const unsigned int flags = 0 | aiProcess_JoinIdenticalVertices
                               | aiProcess_Triangulate
                               | aiProcess_GenSmoothNormals
                               | aiProcess_LimitBoneWeights
                               | aiProcess_SplitLargeMeshes
                               | aiProcess_ImproveCacheLocality
                               | aiProcess_RemoveRedundantMaterials
                               | aiProcess_FindDegenerates
                               | aiProcess_FindInvalidData
                               | aiProcess_GenUVCoords;

    LOG_INFO("MeshConvert: importing file ", filePath);

    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(filePath.c_str(), flags);

    if (!scene || !scene->HasMeshes())
    {
        LOG_ERROR("MeshConvert: unable to load ", filePath);
        return false;
    }

    meshData.meshes.reserve(scene->mNumMeshes);
    meshData.boxes.reserve(scene->mNumMeshes);

    u32 indexOffset = 0;
    u32 vertexOffset = 0;

    for (auto i = 0; i != scene->mNumMeshes; i++)
    {
        // LOG_INFO("Converting meshes ", i + 1, "/", scene->mNumMeshes, "...");
        meshData.meshes.push_back(convertAIMesh(scene->mMeshes[i], meshData, indexOffset, vertexOffset));
    }

    recalculateBoundingBoxes(meshData);

    return true;
}

bool ConvertToMeshFile(const std::string& dstMeshFilePath, const std::string& dstDrawDataFilePath, const std::string& srcFilePath)
{
    LOG_INFO("MeshConvert: converting ", srcFilePath, " and saving to ", dstMeshFilePath,
             " and ", dstDrawDataFilePath);

    MeshData meshData;
    if (!loadFileToMeshData(meshData, srcFilePath))
    {
        return false;
    }

    auto drawData = createMeshDrawData(meshData);

    if (!saveMeshData(dstMeshFilePath, meshData) || !saveDrawData(dstDrawDataFilePath, drawData))
    {
        return false;
    }

    LOG_INFO("Conversion complete!");

    return true;
}

} // namespace MeshConvert

} // namespace Suoh
