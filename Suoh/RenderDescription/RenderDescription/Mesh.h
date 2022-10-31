#pragma once

#include "BoundingBox.h"

#include <string>

constexpr auto MAX_LODS = 8;
constexpr auto MAX_STREAMS = 8;

struct Mesh
{
    u32 vertexCount;

    u32 vertexOffset{0};
    u32 indexOffset{0};

    // Vertex attributes.
    u32 streamCount{0};
    u32 streamOffset[MAX_STREAMS]{0};
    u32 streamElementSize[MAX_STREAMS]{0};

    u32 lodCount{0};
    // Last element is a marker for the size of the last LOD indices.
    u32 lodOffset[MAX_LODS + 1]{0};

    inline u32 GetLODIndicesCount(u32 lod) const
    {
        return lodOffset[lod + 1] - lodOffset[lod];
    }
};

struct MeshData
{
    std::vector<float> vertexData;
    std::vector<u32> indexData;
    std::vector<Mesh> meshes;
    std::vector<BoundingBox> boundingBoxes;
};

struct DrawData
{
    u32 meshIndex;
    u32 materialIndex;
    u32 LOD;
    u32 vertexOffset;
    u32 indexOffset;

    // Transform index in scene.
    u32 transformIndex;
};

struct MeshFileHeader
{
    u32 magicNumber;

    u32 meshCount;

    // Raw data sizes, not vertex/index count.
    u32 vertexDataSize;
    u32 indexDataSize;

    // Offset to start of mesh data, i.e vertex and index data
    u32 dataBlockStartOffset;
};

static_assert(sizeof(BoundingBox) == (sizeof(float) * 6),
              "Size of Bounding Box must be 6 * sizeof floats!");
static_assert(sizeof(DrawData) == (sizeof(u32) * 6), "Size of DrawData must be 6 * 32 bits!");

// Create 1 DrawData per mesh in MeshData.
std::vector<DrawData> CreateMeshDrawData(const MeshData& meshData);

void RecalculateBoundingBoxes(MeshData& meshData);

bool SaveMeshData(const std::string& fileName, const MeshData& meshData);
bool saveDrawData(const std::string& fileName, const std::vector<DrawData>& drawData);

MeshFileHeader LoadMeshData(const std::string& fileName, MeshData& meshData);
std::vector<DrawData> LoadDrawData(const std::string& fileName);
