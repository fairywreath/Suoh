//#pragma once
//
//#include <algorithm>
//#include <execution>
//#include <filesystem>
//#include <fstream>
//#include <string>
//
//#include <assimp/Importer.hpp>
//#include <assimp/material.h>
//#include <assimp/pbrmaterial.h>
//#include <assimp/postprocess.h>
//#include <assimp/scene.h>
//
//#include <Graphics/Scene/Material.h>
//#include <Graphics/Scene/Scene.h>
//#include <Graphics/Scene/VertexData.h>
//
//namespace Suoh
//{
//
//namespace SceneConvert
//{
//
//struct SceneConfig
//{
//    std::string fileName;
//    std::string outputMesh;
//    std::string outputScene;
//    std::string outputMaterials;
//
//    float scale;
//    bool calculateLODs;
//    bool mergeInstances;
//};
//
//MaterialDescription convertAIMaterialToMaterialDescription(const aiMaterial* M, std::vector<std::string>& files,
//                                                           std::vector<std::string>& opacityMaps);
//
//void processLods(std::vector<u32>& indices, std::vector<float>& vertices, std::vector<std::vector<u32>>& outLods);
//
//Mesh convertAIMesh(const aiMesh* aimesh, const SceneConfig& config, MeshData& meshData, u32& indexOffset, u32& vertexOffset);
//
//void processScene(const SceneConfig& config);
//
//} // namespace SceneConvert
//
//} // namespace Suoh
