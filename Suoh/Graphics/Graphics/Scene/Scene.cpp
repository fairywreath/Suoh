#include "Scene.h"
#include "SaveUtils.h"

#include <Core/Logger.h>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <filesystem>

namespace fs = std::filesystem;


namespace Suoh
{

namespace Scene
{

int addNode(SceneContext& scene, int parent, int level)
{
    int node = (int)scene.hierarchy.size();
    {
        // XXX: resize aux arrays (local/global etc.)
        scene.localTransforms.push_back(glm::mat4(1.0f));
        scene.globalTransforms.push_back(glm::mat4(1.0f));
    }

    scene.hierarchy.push_back({
        .parent = parent,
        .lastSibling = -1,
    });

    if (parent > -1)
    {
        int firstChild = scene.hierarchy[parent].firstChild;

        if (firstChild == -1)
        {
            scene.hierarchy[parent].firstChild = node;
            scene.hierarchy[node].lastSibling = node;
        }
        else
        {
            int dest = scene.hierarchy[firstChild].lastSibling;
            if (dest <= -1)
            {
                // no cached lastSibling, iterate nextSibling indices
                for (dest = firstChild; scene.hierarchy[dest].nextSibling != -1; dest = scene.hierarchy[dest].nextSibling)
                    ;
            }

            scene.hierarchy[dest].nextSibling = node;
            scene.hierarchy[firstChild].lastSibling = node;
        }
    }
    scene.hierarchy[node].level = level;
    scene.hierarchy[node].nextSibling = -1;
    scene.hierarchy[node].firstChild = -1;

    return node;
}

void markAsChanged(SceneContext& scene, int node)
{
    int level = scene.hierarchy[node].level;
    scene.changedAtThisFrame[level].push_back(node);

    // XXX: use non-recursive iteration with aux stack
    for (auto s = scene.hierarchy[node].firstChild; s != -1; s = scene.hierarchy[s].nextSibling)
        markAsChanged(scene, s);
}

int findNodeByName(const SceneContext& scene, const std::string& name)
{
    // Extremely simple linear search without any hierarchy reference
    // To support DFS/BFS searches separate traversal routines are needed
    for (auto i = 0; i < scene.localTransforms.size(); i++)
    {
        if (scene.namesMap.contains(i))
        {
            int strID = scene.namesMap.at(i);
            if (strID > -1)
                if (scene.names[strID] == name)
                    return (int)i;
        }
    }

    return -1;
}

int getNodeLevel(const SceneContext& scene, int n)
{
    int level = -1;
    // for (auto p = 0; p != -1; p = scene.hierarchy[p].parent, level++)
    //     ;
    auto p = n;
    while (p != -1)
    {
        p = scene.hierarchy[p].parent;
        level++;
    }

    return level;
}

void recalculateGlobalTransforms(SceneContext& scene)
{
    if (!scene.changedAtThisFrame[0].empty())
    {
        int c = scene.changedAtThisFrame[0][0];
        scene.globalTransforms[c] = scene.localTransforms[c];
        scene.changedAtThisFrame[0].clear();
    }

    for (auto i = 1; i < MAX_SCENE_LEVEL && (!scene.changedAtThisFrame[i].empty()); i++)
    {
        for (const auto& c : scene.changedAtThisFrame[i])
        {
            auto p = scene.hierarchy[c].parent;
            scene.globalTransforms[c] = scene.globalTransforms[p] * scene.localTransforms[c];
        }
        scene.changedAtThisFrame[i].clear();
    }
}

/*
 * Scene loading/saving.
 */
bool loadScene(const std::string& fileName, SceneContext& scene)
{
    std::ifstream file(fileName, std::ios::out | std::ios::binary);
    fs::path path(fileName);

    if (!file)
    {
        LOG_ERROR("loadScene: failed to open ", fs::absolute(path));
        return false;
    }

    u32 size = 0;
    file.read((char*)&size, sizeof(size));

    scene.hierarchy.resize(size);
    scene.globalTransforms.resize(size);
    scene.localTransforms.resize(size);

    // XXX: check > -1
    // XXX: recalculate changedAtThisLevel() - find max depth of a node [or save scene.maxLevel]
    // XXX: get underlying type for transforms and hierarcy array instead of manually giving the type
    file.read((char*)scene.localTransforms.data(), sizeof(mat4) * size);
    file.read((char*)scene.globalTransforms.data(), sizeof(mat4) * size);
    file.read((char*)scene.hierarchy.data(), sizeof(SceneHierarchy) * size);

    loadMap(file, scene.materialsMap);
    loadMap(file, scene.meshesMap);

    if (!file.eof())
    {
        loadMap(file, scene.namesMap);
        loadStringArray(file, scene.names);
        loadStringArray(file, scene.materialNames);
    }

    if (!file.good())
    {
        LOG_ERROR("loadScene: failed to read scene data - ", fileName);
        return false;
    }

    file.close();

    return true;
}

bool saveScene(const std::string& fileName, SceneContext& scene)
{
    std::ofstream file(fileName, std::ios::out | std::ios::binary);
    if (!file)
    {
        LOG_ERROR("saveScene: failed to open file ", fileName);
        return false;
    }

    const u32 nodeCount = (u32)scene.hierarchy.size();
    file.write((char*)&nodeCount, sizeof(nodeCount));

    file.write((char*)scene.localTransforms.data(), sizeof(mat4) * nodeCount);
    file.write((char*)scene.globalTransforms.data(), sizeof(mat4) * nodeCount);
    file.write((char*)scene.hierarchy.data(), sizeof(SceneHierarchy) * nodeCount);

    saveMap(file, scene.materialsMap);
    saveMap(file, scene.meshesMap);

    if (!scene.names.empty() && !scene.namesMap.empty())
    {
        saveMap(file, scene.namesMap);
        saveStringArray(file, scene.names);
        saveStringArray(file, scene.materialNames);
    }

    file.close();

    return true;
}

void dumpTransforms(const std::string& fileName, const SceneContext& scene)
{
    //     FILE* f = fopen(fileName, "a+");
    //     for (size_t i = 0; i < scene.localTransforms.size(); i++)
    //     {
    //         fprintf(f, "Node[%d].localTransform: ", (int)i);
    //         fprintfMat4(f, scene.localTransforms[i]);
    //         fprintf(f, "Node[%d].globalTransform: ", (int)i);
    //         fprintfMat4(f, scene.globalTransforms[i]);
    //         fprintf(f, "Node[%d].globalDet = %f; localDet = %f\n", (int)i, glm::determinant(scene.globalTransforms[i]),
    //                 glm::determinant(scene.localTransforms[i]));
    //     }
    //     fclose(f);
}

void printChangedNodes(const SceneContext& scene)
{
    // for (int i = 0; i < MAX_NODE_LEVEL && (!scene.changedAtThisFrame_[i].empty()); i++)
    // {
    //     printf("Changed at level(%d):\n", i);

    //     for (const int& c : scene.changedAtThisFrame_[i])
    //     {
    //         int p = scene.hierarchy[c].parent;
    //         // scene.globalTransforms[c] = scene.globalTransforms[p] * scene.localTransforms[c];
    //         printf(" Node %d. Parent = %d; LocalTransform: ", c, p);
    //         fprintfMat4(stdout, scene.localTransforms[i]);
    //         if (p > -1)
    //         {
    //             printf(" ParentGlobalTransform: ");
    //             fprintfMat4(stdout, scene.globalTransforms[p]);
    //         }
    //     }
    // }
}

namespace
{

/*
 * Shift all hierarchy components in the nodes.
 * XXX: use a non recursive implementation?
 */
void shiftNodes(SceneContext& scene, int startOffset, int nodeCount, int shiftAmount)
{
    auto shiftNode = [shiftAmount](SceneHierarchy& node) {
        if (node.parent > -1)
            node.parent += shiftAmount;
        if (node.firstChild > -1)
            node.firstChild += shiftAmount;
        if (node.nextSibling > -1)
            node.nextSibling += shiftAmount;
        if (node.lastSibling > -1)
            node.lastSibling += shiftAmount;
        // node->level_ does not have to be shifted
    };

    // If there are too many nodes, we can use std::execution::par with std::transform
    //	std::transform(scene.hierarchy.begin() + startOffset, scene.hierarchy.begin() + nodeCount, scene.hierarchy.begin() + startOffset,
    // shiftNode);

    //	for (auto i = scene.hierarchy.begin() + startOffset ; i != scene.hierarchy.begin() + nodeCount ; i++)
    //		shiftNode(*i);

    for (int i = 0; i < nodeCount; i++)
        shiftNode(scene.hierarchy[i + startOffset]);
}

using ItemMap = std::unordered_map<u32, u32>;

// Add the items from otherMap shifting indices and values along the way
void mergeMaps(ItemMap& m, const ItemMap& otherMap, int indexOffset, int itemOffset)
{
    for (const auto& i : otherMap)
        m[i.first + indexOffset] = i.second + itemOffset;
}

/** A rather long algorithm (and the auxiliary routines) to delete a number of scene nodes from the hierarchy */
/* */

// Add an index to a sorted index array
static void addUniqueIdx(std::vector<u32>& v, u32 index)
{
    if (!std::binary_search(v.begin(), v.end(), index))
        v.push_back(index);
}

// Recurse down from a node and collect all nodes which are already marked for deletion
static void collectNodesToDelete(const SceneContext& scene, int node, std::vector<u32>& nodes)
{
    for (int n = scene.hierarchy[node].firstChild; n != -1; n = scene.hierarchy[n].nextSibling)
    {
        addUniqueIdx(nodes, n);
        collectNodesToDelete(scene, n, nodes);
    }
}

int findLastNonDeletedItem(const SceneContext& scene, const std::vector<int>& newIndices, int node)
{
    // we have to be more subtle:
    //   if the (newIndices[firstChild] == -1), we should follow the link and extract the last non-removed item
    //   ..
    if (node == -1)
        return -1;

    return (newIndices[node] == -1) ? findLastNonDeletedItem(scene, newIndices, scene.hierarchy[node].nextSibling) : newIndices[node];
}

void shiftMapIndices(std::unordered_map<u32, u32>& items, const std::vector<int>& newIndices)
{
    std::unordered_map<u32, u32> newItems;
    for (const auto& m : items)
    {
        int newIndex = newIndices[m.first];
        if (newIndex != -1)
            newItems[newIndex] = m.second;
    }
    items = newItems;
}

} // namespace

/**
    There are different use cases for scene merging.
    The simplest one is the direct "gluing" of multiple scenes into one [all the material lists and mesh lists are merged and indices in all
   scene nodes are shifted appropriately] The second one is creating a "grid" of objects (or scenes) with the same material and mesh sets.
    For the second use case we need two flags: 'mergeMeshes' and 'mergeMaterials' to avoid shifting mesh indices
*/
void mergeScenes(SceneContext& scene, const std::vector<SceneContext*>& scenes, const std::vector<glm::mat4>& rootTransforms,
                 const std::vector<u32>& meshCounts, bool mergeMeshes, bool mergeMaterials)
{
    // Create new root node
    scene.hierarchy = {{
        .parent = -1,
        .firstChild = 1,
        .nextSibling = -1,
        .lastSibling = -1,
        .level = 0,
    }};

    scene.namesMap[0] = 0;
    scene.names = {"NewRoot"};

    scene.localTransforms.push_back(glm::mat4(1.f));
    scene.globalTransforms.push_back(glm::mat4(1.f));

    if (scenes.empty())
        return;

    int offs = 1;
    int meshOffs = 0;
    int nameOffs = (int)scene.names.size();
    int materialOfs = 0;
    auto meshCount = meshCounts.begin();

    if (!mergeMaterials)
        scene.materialNames = scenes[0]->materialNames;

    // XXX FIXME: too much logic (for all the components in a scene, though mesh data and materials go separately - there are dedicated data
    // lists)
    for (const auto* s : scenes)
    {
        mergeVectors(scene.localTransforms, s->localTransforms);
        mergeVectors(scene.globalTransforms, s->globalTransforms);

        mergeVectors(scene.hierarchy, s->hierarchy);

        mergeVectors(scene.names, s->names);
        if (mergeMaterials)
            mergeVectors(scene.materialNames, s->materialNames);

        int nodeCount = (int)s->hierarchy.size();

        shiftNodes(scene, offs, nodeCount, offs);

        mergeMaps(scene.meshesMap, s->meshesMap, offs, mergeMeshes ? meshOffs : 0);
        mergeMaps(scene.materialsMap, s->materialsMap, offs, mergeMaterials ? materialOfs : 0);
        mergeMaps(scene.namesMap, s->namesMap, offs, nameOffs);

        offs += nodeCount;

        materialOfs += (int)s->materialNames.size();
        nameOffs += (int)s->names.size();

        if (mergeMeshes)
        {
            meshOffs += *meshCount;
            meshCount++;
        }
    }

    // fixing 'nextSibling' fields in the old roots (zero-index in all the scenes)
    offs = 1;
    int idx = 0;
    for (const auto* s : scenes)
    {
        int nodeCount = (int)s->hierarchy.size();
        bool isLast = (idx == scenes.size() - 1);
        // calculate new next sibling for the old scene roots
        int next = isLast ? -1 : offs + nodeCount;
        scene.hierarchy[offs].nextSibling = next;
        // attach to new root
        scene.hierarchy[offs].parent = 0;

        // transform old root nodes, if the transforms are given
        if (!rootTransforms.empty())
            scene.localTransforms[offs] = rootTransforms[idx] * scene.localTransforms[offs];

        offs += nodeCount;
        idx++;
    }

    // now shift levels of all nodes below the root
    for (auto i = scene.hierarchy.begin() + 1; i != scene.hierarchy.end(); i++)
        i->level++;
}

void dumpSceneToDot(const std::string& fileName, const SceneContext& scene, int* visited)
{
    // FILE* f = fopen(fileName, "w");
    // fprintf(f, "digraph G\n{\n");
    // for (size_t i = 0; i < scene.globalTransforms.size(); i++)
    // {
    //     std::string name = "";
    //     std::string extra = "";
    //     if (scene.namesMap.contains(i))
    //     {
    //         int strID = scene.namesMap.at(i);
    //         name = scene.names[strID];
    //     }
    //     if (visited)
    //     {
    //         if (visited[i])
    //             extra = ", color = red";
    //     }
    //     fprintf(f, "n%d [label=\"%s\" %s]\n", (int)i, name.c_str(), extra.c_str());
    // }
    // for (size_t i = 0; i < scene.hierarchy.size(); i++)
    // {
    //     int p = scene.hierarchy[i].parent;
    //     if (p > -1)
    //         fprintf(f, "\t n%d -> n%d\n", p, (int)i);
    // }
    // fprintf(f, "}\n");
    // fclose(f);
}

// Approximately an O ( N * Log(N) * Log(M)) algorithm (N = scene.size, M = nodesToDelete.size) to delete a collection of nodes from scene
// graph
void deleteSceneNodes(SceneContext& scene, const std::vector<u32>& nodesToDelete)
{
    // 0) Add all the nodes down below in the hierarchy
    auto indicesToDelete = nodesToDelete;
    for (auto i : indicesToDelete)
        collectNodesToDelete(scene, i, indicesToDelete);

    // aux array with node indices to keep track of the moved ones [moved = [](node) { return (node != nodes[node]); ]
    std::vector<int> nodes(scene.hierarchy.size());
    std::iota(nodes.begin(), nodes.end(), 0);

    // 1.a) Move all the indicesToDelete to the end of 'nodes' array (and cut them off, a variation of swap'n'pop for multiple elements)
    auto oldSize = nodes.size();
    eraseSelected(nodes, indicesToDelete);

    // 1.b) Make a newIndices[oldIndex] mapping table
    std::vector<int> newIndices(oldSize, -1);
    for (int i = 0; i < nodes.size(); i++)
        newIndices[nodes[i]] = i;

    // 2) Replace all non-null parent/firstChild/nextSibling pointers in all the nodes by new positions
    auto nodeMover = [&scene, &newIndices](SceneHierarchy& h) {
        return SceneHierarchy{.parent = (h.parent != -1) ? newIndices[h.parent] : -1,
                              .firstChild = findLastNonDeletedItem(scene, newIndices, h.firstChild),
                              .nextSibling = findLastNonDeletedItem(scene, newIndices, h.nextSibling),
                              .lastSibling = findLastNonDeletedItem(scene, newIndices, h.lastSibling)};
    };
    std::transform(scene.hierarchy.begin(), scene.hierarchy.end(), scene.hierarchy.begin(), nodeMover);

    // 3) Finally throw away the hierarchy items
    eraseSelected(scene.hierarchy, indicesToDelete);

    // 4) As in mergeScenes() routine we also have to adjust all the "components" (i.e., meshesMap, materials, names and transformations)

    // 4a) Transformations are stored in arrays, so we just erase the items as we did with the scene.hierarchy
    eraseSelected(scene.localTransforms, indicesToDelete);
    eraseSelected(scene.globalTransforms, indicesToDelete);

    // 4b) All the maps should change the key values with the newIndices[] array
    shiftMapIndices(scene.meshesMap, newIndices);
    shiftMapIndices(scene.materialsMap, newIndices);
    shiftMapIndices(scene.namesMap, newIndices);

    // 5) scene node names list is not modified, but in principle it can be (remove all non-used items and adjust the namesMap map)
    // 6) Material names list is not modified also, but if some materials fell out of use
}

} // namespace Scene

} // namespace Suoh
