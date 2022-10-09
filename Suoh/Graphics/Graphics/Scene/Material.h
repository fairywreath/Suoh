#pragma once

#include <string>
#include <vector>

#include <Core/Types.h>
#include <Core/Math.h>

#include "SaveUtils.h"

namespace Suoh
{

enum MaterialFlags : u8
{
    MaterialFlags_CastShadow = 0x1,
    MaterialFlags_ReceiveShadow = 0x2,
    MaterialFlags_Transparent = 0x4,
};

constexpr const u64 INVALID_TEXTURE = 0xFFFFFFFF;

struct PACKED_STRUCT MaterialDescription final
{
    gpuvec4 emissiveColor = {0.0f, 0.0f, 0.0f, 0.0f};
    gpuvec4 albedoColor = {1.0f, 1.0f, 1.0f, 1.0f};

    // UV anisotropic roughness (isotropic lighting models use only the first value). ZW values are ignored
    gpuvec4 roughness = {1.0f, 1.0f, 0.0f, 0.0f};
    float transparencyFactor = 1.0f;
    float alphaTest = 0.0f;
    float metallicFactor = 0.0f;

    u32 flags = MaterialFlags_CastShadow | MaterialFlags_ReceiveShadow;

    // maps
    u64 ambientOcclusionMap = INVALID_TEXTURE;
    u64 emissiveMap = INVALID_TEXTURE;
    u64 albedoMap = INVALID_TEXTURE;

    /// Occlusion (R), Roughness (G), Metallic (B) https://github.com/KhronosGroup/glTF/issues/857
    u64 metallicRoughnessMap = INVALID_TEXTURE;
    u64 normalMap = INVALID_TEXTURE;
    u64 opacityMap = INVALID_TEXTURE;
};

static_assert(sizeof(MaterialDescription) % 16 == 0, "MaterialDescription should be padded to 16 bytes");

void saveMaterials(const std::string& fileName, const std::vector<MaterialDescription>& materials, const std::vector<std::string>& files);
void loadMaterials(const std::string&, std::vector<MaterialDescription>& materials, std::vector<std::string>& files);

void mergeMaterialLists(
    // Input:
    const std::vector<std::vector<MaterialDescription>*>& oldMaterials, // all materials
    const std::vector<std::vector<std::string>*>& oldTextures,          // all textures from all material lists
    // Output:
    std::vector<MaterialDescription>& allMaterials,
    std::vector<std::string>& newTextures // all textures (merged from oldTextures, only unique items)
);

} // namespace Suoh
