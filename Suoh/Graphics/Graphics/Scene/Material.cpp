#include "Material.h"
#include "SaveUtils.h"

#include <Core/Logger.h>

#include <unordered_map>
#include <fstream>


namespace Suoh
{

void saveMaterials(const std::string& fileName, const std::vector<MaterialDescription>& materials, const std::vector<std::string>& files)
{
    std::ofstream file(fileName, std::ios::out | std::ios::binary);
    if (!file)
    {
        LOG_ERROR("saveMaterials: failed to open file ", fileName);
        return;
    }

    //LOG_DEBUG("Saving material file: ", )

    u32 size = (u32)materials.size();
    file.write((const char*)&size, sizeof(size));
    file.write((const char*)materials.data(), sizeof(MaterialDescription) * size);
    saveStringArray(file, files);

    file.close();
}

void loadMaterials(const std::string& fileName, std::vector<MaterialDescription>& materials, std::vector<std::string>& files)
{
    std::ifstream file(fileName, std::ios::out | std::ios::binary);
    if (!file)
    {
        LOG_ERROR("loadMaterials: failed to open file ", fileName);
        return;
    }

    u32 size;
    file.read((char*)&size, sizeof(size));
    materials.resize(size);
    file.read((char*)materials.data(), sizeof(MaterialDescription) * materials.size());
    loadStringArray(file, files);
    
    file.close();
}

void mergeMaterialLists(const std::vector<std::vector<MaterialDescription>*>& oldMaterials,
                        const std::vector<std::vector<std::string>*>& oldTextures, std::vector<MaterialDescription>& allMaterials,
                        std::vector<std::string>& newTextures)
{
    // map texture names to indices in newTexturesList (calculated as we fill the newTexturesList)
    std::unordered_map<std::string, int> newTextureNames;
    std::unordered_map<int, int>
        materialToTextureList; // direct MaterialDescription usage as a key is impossible, so we use its index in the allMaterials array

    // Create combined material list [no hashing of materials, just straightforward merging of all lists]
    int midx = 0;
    for (const std::vector<MaterialDescription>* ml : oldMaterials)
    {
        for (const MaterialDescription& m : *ml)
        {
            allMaterials.push_back(m);
            materialToTextureList[allMaterials.size() - 1] = midx;
        }

        midx++;
    }

    // Create one combined texture list
    for (const std::vector<std::string>* tl : oldTextures)
        for (const std::string& file : *tl)
        {
            newTextureNames[file] = addUnique(newTextures, file);
        }

    // Lambda to replace textureID by a new "version" (from global list)
    auto replaceTexture = [&materialToTextureList, &oldTextures, &newTextureNames](int m, u64* textureID) {
        if (*textureID < INVALID_TEXTURE)
        {
            auto listIdx = materialToTextureList[m];
            auto texList = oldTextures[listIdx];
            const std::string& texFile = (*texList)[*textureID];
            *textureID = (u64)(newTextureNames[texFile]);
        }
    };

    for (size_t i = 0; i < allMaterials.size(); i++)
    {
        auto& m = allMaterials[i];
        replaceTexture(i, &m.ambientOcclusionMap);
        replaceTexture(i, &m.emissiveMap);
        replaceTexture(i, &m.albedoMap);
        replaceTexture(i, &m.metallicRoughnessMap);
        replaceTexture(i, &m.normalMap);
    }
}

} // namespace Suoh
