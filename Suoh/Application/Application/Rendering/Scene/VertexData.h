#pragma once

#include <vector>

#include <Core/Math.h>
#include <Core/Types.h>

namespace Suoh
{

static constexpr u32 MAX_LODS = 8;
static constexpr u32 MAX_STREAMS = 8;

struct Mesh
{
    u32 lodCount{1};
    u32 streamCount{0};

    u32 materialID{0};

    u32 meshSize{0};

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

    inline u32 getLodIndicesCount(u32 lod)
    {
        return lodOffset[lod + 1] - lodOffset[lod];
    }

    u32 streamOffset[MAX_STREAMS]{0};
    u32 streamElementSize[MAX_STREAMS]{0};

    /*
     * XXX: might want to store stream element type, i.e. byte, float, short, etc.
     */
};

struct MeshFileHeader
{
    /*
     * File signature.
     */
    u32 magicNumber;

    u32 meshCount;

    /*
     * Offset to start of the mesh data.
     */
    u32 dataBlockStartOffset;

    u32 vertexDataSize;
    u32 indexDataSize;
};

struct DrawData
{
    u32 meshIndex;
    u32 materialIndex;
    u32 LOD;
    u32 indexOffset;
    u32 vertexOffset;
    u32 transformIndex;
};

struct MeshData
{
    std::vector<u32> indexData;
    std::vector<float> vertexData;
    std::vector<Mesh> meshes;
    std::vector<BoundingBox> boxes;
};

} // namespace Suoh