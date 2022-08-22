#pragma once

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <meshoptimizer.h>

#include "../Scene/VertexData.h"

namespace Suoh
{

namespace MeshConvert
{

bool loadFileToMesh(MeshData& mesh, const std::string& filePath);

}

} // namespace Suoh
