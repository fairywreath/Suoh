#pragma once

#include <Core/Types.h>

#include <unordered_map>
#include <vector>

namespace Suoh
{

namespace Scene
{

constexpr auto MAX_SCENE_LEVEL = 16;

struct SceneHierarchy
{
    int parent;
    int firstChild;
    int nextSibling;
    int lastSibling;
    int level;
};

struct SceneData
{
    std::vector<mat4> localTransforms;
    std::vector<mat4> globalTransforms;

    std::vector<int> changedAtThisFrame[MAX_SCENE_LEVEL];

    std::vector<SceneHierarchy> hierarchy;

    std::unordered_map<u32, u32> meshesMap;
    std::unordered_map<u32, u32> materialsMap;
    std::unordered_map<u32, u32> namesMap;

    std::vector<std::string> names;
    std::vector<std::string> materialNames;
};

} // namespace Scene

} // namespace Suoh
