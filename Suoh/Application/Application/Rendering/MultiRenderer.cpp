#include "MultiRenderer.h"

#include <core/Logger.h>

namespace Suoh
{

SceneData::SceneData(VKRenderDevice& renderDevice, const std::string& meshFile, const std::string& sceneFile,
                     const std::string& materialFile, Texture envMap, Texture irradianceMap)
    : renderDevice(&renderDevice), envMapIrradiance(irradianceMap), envMap(envMap)
{
    brdfLUT = renderDevice.createKTXTexture("../../../../../Resources/brdfLUT.ktx");

    loadMaterials(materialFile, materials, textureFiles);

    std::vector<Texture> textures;
    for (const auto& file : textureFiles)
    {
        textures.push_back(renderDevice.createTexture(file));
    }

    allMaterialTextures = fsTextureArrayAttachment(textures);

    const u32 materialsSize = static_cast<u32>(sizeof(MaterialDescription) * materials.size());
    renderDevice.createBuffer(materialsSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, materialsBuffer);
    renderDevice.uploadBufferData(materialsBuffer, materials.data(), materialsSize);

    loadMeshes(meshFile);
    loadScene(sceneFile);
}

void SceneData::loadMeshes(const std::string& meshFile)
{
    MeshFileHeader header = loadMeshData(meshFile, meshData);

    const u32 indexBufferSize = header.indexDataSize;
    u32 vertexBufferSize = header.vertexDataSize;

    const u32 offsetAlignment = renderDevice->getMinStorageBufferOffset();

    if ((vertexBufferSize & (offsetAlignment - 1)) != 0)
    {
        const size_t numFloats = (offsetAlignment - (vertexBufferSize & (offsetAlignment - 1))) / sizeof(float);
        for (size_t i = 0; i != numFloats; i++)
            meshData.vertexData.push_back(0);
        vertexBufferSize = (vertexBufferSize + offsetAlignment) & ~(offsetAlignment - 1);
    }

    Buffer storageBuffer;
    renderDevice->createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                               storageBuffer);
    renderDevice->uploadBufferData(storageBuffer, meshData.vertexData.data(), 0, vertexBufferSize);
    renderDevice->uploadBufferData(storageBuffer, meshData.indexData.data(), vertexBufferSize, indexBufferSize);

    vertexBuffer = BufferAttachment{
        .descriptorInfo = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .shaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        .buffer = storageBuffer,
        .offset = 0,
        .size = vertexBufferSize,
    };

    indexBuffer = BufferAttachment{
        .descriptorInfo = {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
            .shaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        .buffer = storageBuffer,
        .offset = vertexBufferSize,
        .size = indexBufferSize,
    };
}

void SceneData::loadScene(const std::string& sceneFile)
{
    Scene::loadScene(sceneFile, scene);

    // prepare draw data buffer
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

    renderDevice->createBuffer(shapes.size() * sizeof(mat4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                               transformsBuffer);

    recalculateAllTransforms();
    uploadGlobalTransforms();
}

void SceneData::updateMaterial(int matIdx)
{
    renderDevice->uploadBufferData(materialsBuffer, materials.data() + matIdx, sizeof(MaterialDescription));
}

void SceneData::convertGlobalToShapeTransforms()
{
    // fill the shapeTransforms array from globalTransforms of scene data
    size_t i = 0;
    for (const auto& c : shapes)
        shapeTransforms[i++] = scene.globalTransforms[c.transformIndex];
}

void SceneData::recalculateAllTransforms()
{
    // force recalculation of global transformations
    Scene::markAsChanged(scene, 0);
    Scene::recalculateGlobalTransforms(scene);
}

void SceneData::uploadGlobalTransforms()
{
    convertGlobalToShapeTransforms();
    renderDevice->uploadBufferData(transformsBuffer, shapeTransforms.data(), transformsBuffer.size);
}

MultiRenderer::MultiRenderer(VKRenderDevice& renderDevice, SceneData& sceneData, const std::string& vertexShaderFile,
                             const std::string& fragmentShaderFile, const std::vector<Texture>& outputs, RenderPass screenRenderPass,
                             const std::vector<BufferAttachment>& auxBuffers, const std::vector<TextureAttachment>& auxTextures)
    : Renderer(renderDevice, 1420, 720), mSceneData(sceneData)
{
    const u32 indirectDataSize = (u32)mSceneData.shapes.size() * sizeof(VkDrawIndirectCommand);

    const size_t imageCount = mRenderDevice->getSwapchainImageCount();
    mUniformBuffers.resize(imageCount);
    mShapes.resize(imageCount);
    mIndirectBuffers.resize(imageCount);
    mDescriptorSets.resize(imageCount);

    const u32 shapesSize = (u32)mSceneData.shapes.size() * sizeof(DrawData);
    constexpr u32 uniformBufferSize = sizeof(mUbo);

    std::vector<TextureAttachment> textureAttachments;
    if (mSceneData.envMap.width)
        textureAttachments.push_back(fsTextureAttachment(mSceneData.envMap));
    else
        LOG_DEBUG("No env map!");

    if (mSceneData.envMapIrradiance.width)
        textureAttachments.push_back(fsTextureAttachment(mSceneData.envMapIrradiance));
    else
        LOG_DEBUG("No irr map!");

    if (mSceneData.brdfLUT.width)
        textureAttachments.push_back(fsTextureAttachment(mSceneData.brdfLUT));
    else
        LOG_DEBUG("No brdf lut!");

    for (const auto& t : auxTextures)
        textureAttachments.push_back(t);

    DescriptorSetInfo dsInfo = {
		.buffers = {
			uniformBufferAttachment(Buffer {},         0, uniformBufferSize, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
			mSceneData.vertexBuffer,
			mSceneData.indexBuffer,
			storageBufferAttachment(Buffer {},         0, shapesSize, VK_SHADER_STAGE_VERTEX_BIT),
			storageBufferAttachment(mSceneData.materialsBuffer,    0, (u32)mSceneData.materialsBuffer.size, VK_SHADER_STAGE_FRAGMENT_BIT),
			storageBufferAttachment(mSceneData.transformsBuffer,  0, (u32)mSceneData.transformsBuffer.size, VK_SHADER_STAGE_VERTEX_BIT),
		},
		.textures = textureAttachments,
		.textureArrays = { mSceneData.allMaterialTextures }
	};

    for (const auto& b : auxBuffers)
        dsInfo.buffers.push_back(b);

    mDescriptorSetLayout = mRenderDevice->createDescriptorSetLayout(dsInfo);
    mDescriptorPool = mRenderDevice->createDescriptorPool(dsInfo, imageCount);

    for (int i = 0; i < imageCount; i++)
    {
        mRenderDevice->createBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mUniformBuffers[i]);
        mRenderDevice->createBuffer(indirectDataSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mIndirectBuffers[i]);

        updateIndirectBuffers(i);
        
        mRenderDevice->createBuffer(shapesSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mShapes[i]);
        mRenderDevice->uploadBufferData(mShapes[i], mSceneData.shapes.data(), shapesSize);

        dsInfo.buffers[0].buffer = mUniformBuffers[i];
        dsInfo.buffers[3].buffer = mShapes[i];

        mDescriptorSets[i] = mRenderDevice->createDescriptorSet(mDescriptorPool, mDescriptorSetLayout);
        mRenderDevice->updateDescriptorSet(mDescriptorSets[i], dsInfo);
    }

    // long renderpass initialization
    mRenderDevice->createDepthResources(processingWidth, processingHeight, mDepthImage);
    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = false,
            .clearDepth = false,
            .flags = 0,
        },
        true, mRenderPass.handle);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass.handle, mDepthImage.imageView, mSwapchainFramebuffers);


    const GraphicsPipelineInfo pipelineInfo = initRenderPass(GraphicsPipelineInfo{}, outputs, screenRenderPass, mRenderPass);

    initPipeline({vertexShaderFile, fragmentShaderFile}, pipelineInfo);
}

void MultiRenderer::fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer fb, VkRenderPass rp)
{
    const auto currentImage = mRenderDevice->getSwapchainImageIndex();
    
    Framebuffer = mSwapchainFramebuffers[currentImage];

    beginRenderPass((rp != VK_NULL_HANDLE) ? rp : mRenderPass.handle, (fb != VK_NULL_HANDLE) ? fb : Framebuffer, commandBuffer);

    vkCmdDrawIndirect(commandBuffer, mIndirectBuffers[currentImage].buffer, 0, (u32)mSceneData.shapes.size(),
                      sizeof(VkDrawIndirectCommand));

    vkCmdEndRenderPass(commandBuffer);
}

void MultiRenderer::updateBuffers()
{
    updateUniformBuffer(0, sizeof(mUbo), &mUbo);
}

void MultiRenderer::updateIndirectBuffers(int imageIdx, bool* visibility)
{
    // const auto currentImage = mRenderDevice->getSwapchainImageIndex();

    VkDrawIndirectCommand* data = nullptr;
    mRenderDevice->mapMemory(mIndirectBuffers[imageIdx].allocation, (void**)&data);

    const u32 size = (u32)mSceneData.shapes.size();

    for (u32 i = 0; i != size; i++)
    {
        const u32 j = mSceneData.shapes[i].meshIndex;

        const u32 lod = mSceneData.shapes[i].LOD;
        data[i] = {
            .vertexCount = mSceneData.meshData.meshes[j].getLODIndicesCount(lod),
            .instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
            .firstVertex = 0,
            .firstInstance = i,
        };
    }

    mRenderDevice->unmapMemory(mIndirectBuffers[imageIdx].allocation);
}

} // namespace Suoh