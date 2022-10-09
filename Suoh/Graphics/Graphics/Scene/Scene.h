#pragma once

#include <Core/StrongTypedef.h>
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
    int parent{-1};
    int firstChild{-1};
    int nextSibling{-1};
    int lastSibling{-1};
    int level{0};
};

struct SceneContext
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

int addNode(SceneContext& scene, int parent, int level);

void markAsChanged(SceneContext& scene, int node);

int findNodeByName(const SceneContext& scene, const std::string& name);

inline std::string getNodeName(const SceneContext& scene, int node)
{
    int strID = scene.namesMap.contains(node) ? scene.namesMap.at(node) : -1;
    return (strID > -1) ? scene.names[strID] : std::string();
}

inline void setNodeName(SceneContext& scene, int node, const std::string& name)
{
    u32 stringID = (u32)scene.names.size();
    scene.names.push_back(name);
    scene.namesMap[node] = stringID;
}

int getNodeLevel(const SceneContext& scene, int n);

void recalculateGlobalTransforms(SceneContext& scene);

bool loadScene(const std::string& fileName, SceneContext& scene);
bool saveScene(const std::string& fileName, SceneContext& scene);

void dumpTransforms(const std::string& fileName, const SceneContext& scene);
void printChangedNodes(const SceneContext& scene);

void dumpSceneToDot(const std::string& fileName, const SceneContext& scene, int* visited = nullptr);

void mergeScenes(SceneContext& scene, const std::vector<SceneContext*>& scenes, const std::vector<mat4>& rootTransforms,
                 const std::vector<u32>& meshCounts, bool mergeMeshes = true, bool mergeMaterials = true);

// Delete a collection of nodes from a scenegraph
void deleteSceneNodes(SceneContext& scene, const std::vector<u32>& nodesToDelete);

} // namespace Scene

} // namespace Suoh
