#include "MeshConvert.h"

#include <Core/Logger.h>

namespace Suoh
{

namespace MeshConvert
{

bool loadFileToMesh(MeshData& meshData, const std::string& filePath)
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

    const aiScene* scene = aiImportFile(filePath.c_str(), flags);

    if (!scene || !scene->HasMeshes())
    {
        LOG_ERROR("loadFileToMesh: unable to load ", filePath);
        return false;
    }

    meshData.meshes.reserve(scene->mNumMeshes);
    meshData.boxes.reserve(scene->mNumMeshes);

    for (auto i = 0; i != scene->mNumMeshes; i++)
    {
        LOG_INFO("Converting meshes ", i + 1, "/", scene->mNumMeshes, "...");
        // meshData.meshes.push_back(convertAIMesh(scene->mMeshes[i]));
    }

    // recalculateBoundingBoxes(g_meshData);
}

} // namespace MeshConvert

} // namespace Suoh