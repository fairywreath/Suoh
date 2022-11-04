#include "SceneData.h"

bool SceneData::Load(const std::string& meshFile, const std::string& sceneFile,
                     const std::string& materialFile)
{
    if (!LoadMaterials(materialFile, materials, textureFiles))
    {
        return false;
    }

    auto header = LoadMeshData(meshFile, meshData);
    if (header.meshCount == 0)
    {
        return false;
    }

    if (!LoadScene(sceneFile, scene))
    {
        return false;
    }

    // Setup scene drawdata.
    for (const auto& c : scene.meshesMap)
    {
        auto material = scene.materialsMap.find(c.first);
        if (material == scene.materialsMap.end())
            continue;

        shapes.push_back(DrawData{
            .meshIndex = c.second,
            .materialIndex = material->second,
            .LOD = 0,
            .indexOffset = meshData.meshes[c.second].indexOffset,
            .vertexOffset = meshData.meshes[c.second].vertexOffset,
            .transformIndex = c.first,
        });
    }
    shapeTransforms.resize(shapes.size());

    return true;
}

void SceneData::InitializeGPUResources(RenderDevice* renderDevice)
{
    auto device = renderDevice->device;

    auto brdfLUTImage = renderDevice->CreateKTXTextureImage("../Resources/brdfLUT.ktx");
    auto brdfLUTSampler = device->CreateSampler({});
    brdfLUT = MakeTexture(brdfLUTImage, brdfLUTSampler);

    // Create material textures.
    for (const auto& file : textureFiles)
    {
        auto image = renderDevice->CreateTextureImage(file);
        auto sampler = device->CreateSampler({});
        image->sampler = sampler->sampler;

        materialTextures.push_back(MakeTexture(image, sampler));
    }

    // Create material buffer.
    BufferDesc mbDesc;
    mbDesc.size = materials.size() * sizeof(MaterialDescription);
    mbDesc.usage = BufferUsage::STORAGE_BUFFER;
    materialsBuffer = device->CreateBuffer(mbDesc);
    renderDevice->UploadBufferData(materialsBuffer, materials.data(), mbDesc.size);

    // Create vertex and index buffer.
    auto vertexBufferSize = meshData.vertexData.size() * sizeof(float);
    auto indexBufferSize = meshData.indexData.size() * sizeof(u32);

    // XXX: Properly get this from device.
    const auto offsetAlignment = 64;
    if ((vertexBufferSize & (offsetAlignment - 1)) != 0)
    {
        const size_t numFloats
            = (offsetAlignment - (vertexBufferSize & (offsetAlignment - 1))) / sizeof(float);
        for (size_t i = 0; i != numFloats; i++)
            meshData.vertexData.push_back(0);
        vertexBufferSize = (vertexBufferSize + offsetAlignment) & ~(offsetAlignment - 1);
    }

    BufferDesc sbDesc;
    sbDesc.size = vertexBufferSize + indexBufferSize;
    sbDesc.usage = BufferUsage::STORAGE_BUFFER;
    storageBuffer = device->CreateBuffer(sbDesc);

    renderDevice->UploadBufferData(storageBuffer, meshData.vertexData.data(), vertexBufferSize, 0);
    renderDevice->UploadBufferData(storageBuffer, meshData.indexData.data(), indexBufferSize,
                                   vertexBufferSize);

    // Create scene transforms buffer.
    // XXX: This buffer acts like a "uniform buffer", since it may need frequent writes, hence the
    // cpu access. Maybe have this a uniform buffer instead?
    BufferDesc tbDesc;
    tbDesc.size = shapes.size() * sizeof(mat4);
    tbDesc.usage = BufferUsage::STORAGE_BUFFER;
    tbDesc.cpuAccess = CpuAccessMode::WRITE;
    transformsBuffer = device->CreateBuffer(tbDesc);

    std::vector<ImageHandle> materialImages;
    for (const auto& texture : materialTextures)
        materialImages.push_back(texture.image);

    // Setup descriptor bindings.
    materialImagesDescriptor.images = std::move(materialImages);
    materialImagesDescriptor.descriptorItem.type = vk::DescriptorType::eCombinedImageSampler;
    materialImagesDescriptor.descriptorItem.shaderStageFlags = vk::ShaderStageFlagBits::eFragment;

    vertexBufferDescriptor = MakeVSStorageBufferDescriptor(storageBuffer, vertexBufferSize, 0);
    indexBufferDescriptor
        = MakeVSStorageBufferDescriptor(storageBuffer, indexBufferSize, vertexBufferSize);

    RecalculateTransforms();
    UploadTransforms(device);
}

void SceneData::UploadTransforms(VulkanDevice* device)
{
    UpdateShapeTransforms();

    auto mappedMemory = device->MapBuffer(transformsBuffer);
    memcpy(mappedMemory, shapeTransforms.data(), sizeof(mat4) * shapeTransforms.size());
    device->UnmapBuffer(transformsBuffer);
}

void SceneData::UploadMaterial(VulkanDevice* device, u32 index)
{
    auto mappedMemory = device->MapBuffer(materialsBuffer);
    memcpy((u8*)mappedMemory + (sizeof(MaterialDescription) * index), materials.data() + index,
           sizeof(MaterialDescription));
    device->UnmapBuffer(materialsBuffer);
}

void SceneData::UpdateShapeTransforms()
{
    u32 i = 0;
    for (const auto& shape : shapes)
        shapeTransforms[i++] = scene.globalTransforms[shape.transformIndex];
}

void SceneData::RecalculateTransforms()
{
    MarkAsChanged(scene, 0);
    RecalculateGlobalTransforms(scene);
}
