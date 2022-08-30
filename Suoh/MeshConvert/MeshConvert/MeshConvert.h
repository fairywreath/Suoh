#pragma once

#include <Graphics/Scene/VertexData.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Suoh
{

namespace MeshConvert
{

Mesh convertAIMesh(const aiMesh* aimesh, MeshData& meshData, u32& indexOffset, u32& vertexOffset);
bool loadFileToMeshData(MeshData& mesh, const std::string& filePath);

bool ConvertToMeshFile(const std::string& dstMeshFilePath, const std::string& dstDrawDataFilePath, const std::string& srcFilePath);

} // namespace MeshConvert

} // namespace Suoh
