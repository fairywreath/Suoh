#pragma once
#include <RenderLib/Vulkan/VulkanDevice.h>

#include <RenderDescription/Material.h>
#include <RenderDescription/Mesh.h>
#include <RenderDescription/Scene.h>

#include "RenderDevice.h"
#include "RenderUtils.h"

struct SceneData
{
    // Render descriptions.
    MeshData meshData;
    Scene scene;

    u32 framebufferWidth;
    u32 framebufferHeight;

    std::vector<MaterialDescription> materials;
    std::vector<std::string> textureFiles;

    std::vector<mat4> shapeTransforms;
    std::vector<DrawData> shapes;

    // GPU resources.
    Texture envMap;
    Texture envMapIrradiance;
    Texture brdfLUT;

    BufferHandle materialsBuffer;
    BufferHandle transformsBuffer;

    // Storage buffer for vertex and index data.
    BufferHandle storageBuffer;
    BufferDescriptorItem vertexBufferDescriptor;
    BufferDescriptorItem indexBufferDescriptor;

    std::vector<Texture> materialTextures;
    ImageArrayDescriptorItem materialImagesDescriptor;

    // Loaders.
    bool Load(const std::string& meshFile, const std::string& sceneFile,
              const std::string& materialFile);

    // Scene calculations.
    void UpdateShapeTransforms();
    void RecalculateTransforms();

    // GPU operations.
    void InitializeGPUResources(RenderDevice* renderDevice);
    void UploadTransforms(VulkanDevice* device);
    void UploadMaterial(VulkanDevice* device, u32 index);
};
