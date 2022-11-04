#pragma once

#include <CoreTypes.h>

#include <string>
#include <vector>

constexpr u64 INVALID_TEXTURE = 0xFFFFFFFF;

enum class MaterialFlags : u8
{
    TRANSPARENT = 0x1,
    CAST_SHADOW = 0x2,
    RECEIVE_SHADOW = 0x4,
};

ENUM_CLASS_FLAG_OPERATORS(MaterialFlags);

struct PACKED_STRUCT MaterialDescription
{
    GPUVec4 emissiveColor{0.0f};
    GPUVec4 albedoColor{1.0f};
    // Only first 2 values are used.
    GPUVec4 roughness{1.0f, 1.0f, 0.0f, 0.0f};

    float transparencyFactor{1.0f};
    float alphaTest{1.0f};
    float metallicFactor{0.0f};

    // XXX: Handle flags nicely.
    u32 flags{u32(MaterialFlags::CAST_SHADOW | MaterialFlags::RECEIVE_SHADOW)};

    // Index to texture names/array.
    u64 ambientOcclusionMap{INVALID_TEXTURE};
    u64 emissiveMap{INVALID_TEXTURE};
    u64 albedoMap{INVALID_TEXTURE};
    u64 metallicRoughnessMap{INVALID_TEXTURE};
    u64 normalMap{INVALID_TEXTURE};
    u64 opacityMap{INVALID_TEXTURE};
};

static_assert(sizeof(MaterialDescription) % 16 == 0,
              "MaterialDescription must be padded to 16 bytes!");

bool SaveMaterials(const std::string& fileName, const std::vector<MaterialDescription>& materials,
                   const std::vector<std::string>& files);
bool LoadMaterials(const std::string& fileName, std::vector<MaterialDescription>& materials,
                   std::vector<std::string>& files);

// XXX: Need to handle case where merged texture array size exceeds device shader limits.
void MergeMaterialLists(std::vector<MaterialDescription>& dstMaterials,
                        std::vector<std::string>& dstTextures,
                        const std::vector<std::vector<MaterialDescription>*>& srcMaterials,
                        const std::vector<std::vector<std::string>*>& srcTextures);
