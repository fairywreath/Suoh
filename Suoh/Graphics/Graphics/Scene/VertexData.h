#pragma once

#include <Core/Math.h>
#include <Core/Types.h>

#include <string>
#include <vector>

namespace Suoh
{

static constexpr u32 MAX_LODS = 8;
static constexpr u32 MAX_STREAMS = 8;

struct Mesh
{
    u32 lodCount{1};
    u32 streamCount{0};

    /*
     * The total count of all previous vertices in this mesh file.
     */
    u32 indexOffset{0};
    u32 vertexOffset{0};

    /*
     * Vertex count for all LODs.
     */
    u32 vertexCount{0};

    /*
     * Last element is a marker to the last lod element.
     * used to get the size of the last LOD.
     * See getLodIndicesCount function.
     */
    u32 lodOffset[MAX_LODS]{0};

    inline u32 getLODIndicesCount(u32 lod) const
    {
        return lodOffset[lod + 1] - lodOffset[lod];
    }

    u32 streamOffset[MAX_STREAMS]{0};
    u32 streamElementSize[MAX_STREAMS]{0};

    /*
     * XXX: might want to store stream element type, i.e. byte, float, short, etc.
     */
    // u32 materialID{0};
    // u32 meshSize{0};
};

struct MeshFileHeader
{
    /*
     * File signature.
     */
    u32 magicNumber;

    u32 meshCount;

    /*
     * Offset to start of the mesh data, i.e. index and vertex data.
     */
    u32 dataBlockStartOffset;

    u32 indexDataSize;
    u32 vertexDataSize;
};

struct DrawData
{
    u32 meshIndex;
    u32 materialIndex;
    u32 LOD;
    u32 indexOffset;
    u32 vertexOffset;

    u32 transformIndex;
    // mat4 transform;
};

struct MeshData
{
    std::vector<u32> indexData;
    std::vector<float> vertexData;
    std::vector<Mesh> meshes;
    std::vector<BoundingBox> boxes;
};

void recalculateBoundingBoxes(MeshData& meshData);

std::vector<DrawData> createMeshDrawData(const MeshData& meshData);

MeshFileHeader loadMeshData(const std::string& filePath, MeshData& meshData);
std::vector<DrawData> loadDrawData(const std::string& filePath);

bool saveMeshData(const std::string& filePath, const MeshData& meshData);
bool saveDrawData(const std::string& filePath, const std::vector<DrawData>& drawData);

} // namespace Suoh
