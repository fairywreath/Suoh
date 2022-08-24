#include "MultiMeshRenderer.h"

#include <Core/Logger.h>

namespace Suoh
{

MultiMeshRenderer::MultiMeshRenderer(VKRenderDevice* renderDevice, Window* window)
    : RendererBase(renderDevice, window, Image())
{
}

MultiMeshRenderer::~MultiMeshRenderer()
{
    mRenderDevice->destroyPipeline(mGraphicsPipeline);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyRenderPass(mRenderPass);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);

    mRenderDevice->destroyImage(mDepthImage);

    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }

    for (auto& buffer : mUniformBuffers)
    {
        mRenderDevice->destroyBuffer(buffer);
    }

    for (auto& buffer : mDrawCountBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& buffer : mDrawDataBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& buffer : mIndirectBuffers)
        mRenderDevice->destroyBuffer(buffer);

    mRenderDevice->destroyBuffer(mMaterialBuffer);
    mRenderDevice->destroyBuffer(mStorageBuffer);
}

void MultiMeshRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    beginRenderPass(commandBuffer);

    const auto swapchainImageIndex = mRenderDevice->getSwapchainImageIndex();

    /*
     * XXX: can use vkCmdDrawIndirectCount or vkCmdDrawIndirect
     */
    // vkCmdDrawIndirectCount(commandBuffer, mIndirectBuffers[swapchainImageIndex].buffer, 0,
    //                        mDrawCountBuffers[swapchainImageIndex].buffer, 0,
    //                        mMaxDrawDataCount, sizeof(VkDrawIndirectCommand));
    vkCmdDrawIndirect(commandBuffer, mIndirectBuffers[swapchainImageIndex].buffer, 0, mMaxDrawDataCount, sizeof(VkDrawIndirectCommand));

    vkCmdEndRenderPass(commandBuffer);
}

bool MultiMeshRenderer::loadMeshes(const std::string& meshFile, const std::string& drawDataFile, const std::string& materialFile,
                                   const std::string& vertexShaderFile, const std::string& fragmentShaderFile)
{
    auto meshHeader = loadMeshData(meshFile.c_str(), mMeshData);
    if (meshHeader.meshCount == 0)
    {
        return false;
    }

    mDrawData = loadDrawData(drawDataFile.c_str());
    if (mDrawData.empty())
    {
        return false;
    }
    mMaxDrawDataCount = mDrawData.size();

    const u32 indirectDataSize = mMaxDrawDataCount * sizeof(VkDrawIndirectCommand);
    mMaxDrawDataSize = mMaxDrawDataCount * sizeof(DrawData);
    mMaxMaterialSize = 1024;

    const auto swapchainImageCount = mRenderDevice->getSwapchainImageCount();

    mIndirectBuffers.resize(swapchainImageCount);
    mDrawDataBuffers.resize(swapchainImageCount);
    mDrawCountBuffers.resize(swapchainImageCount);

    /*
     * Create storage buffer for vertices and indices.
     */
    mMaxVertexBufferSize = meshHeader.vertexDataSize;
    mMaxIndexBufferSize = meshHeader.indexDataSize;

    const auto minStorageBufferOffset = mRenderDevice->getMinStorageBufferOffset();
    if ((mMaxVertexBufferSize & (minStorageBufferOffset - 1)) != 0)
    {
        const auto floats = (minStorageBufferOffset - (mMaxVertexBufferSize & (minStorageBufferOffset - 1))) / sizeof(float);
        for (auto i = 0; i < floats; i++)
            mMeshData.vertexData.push_back(0);
        mMaxVertexBufferSize = (mMaxVertexBufferSize + minStorageBufferOffset) & ~(minStorageBufferOffset - 1);
    }
    mIndexBufferOffset = mMaxVertexBufferSize;

    mRenderDevice->createBuffer(mMaxVertexBufferSize + mMaxIndexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mStorageBuffer);
    updateGeometryBuffers(meshHeader.vertexDataSize, meshHeader.indexDataSize, mMeshData.vertexData.data(), mMeshData.indexData.data());

    /*
     * Create material buffer.
     */
    mRenderDevice->createBuffer(mMaxMaterialSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mMaterialBuffer);

    for (auto i = 0; i < swapchainImageCount; i++)
    {
        /*
         * Create indirect buffers.
         */
        mRenderDevice->createBuffer(indirectDataSize, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mIndirectBuffers[i]);
        updateIndirectBuffers(i);

        /*
         * Create draw data buffers.
         */
        mRenderDevice->createBuffer(mMaxDrawDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mDrawDataBuffers[i]);
        updateDrawDataBuffer(i, mMaxDrawDataSize, mDrawData.data());

        /*
         * Create count buffers.
         */
        mRenderDevice->createBuffer(sizeof(u32), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mDrawCountBuffers[i]);
        updateCountBuffer(i, mMaxDrawDataCount);
    }

    /*
     * Create shader uniform buffers.
     */
    createUniformBuffers(sizeof(UniformBuffer));

    /*
     * Create renderpass, framebuffers and depth image.
     * Note the renderpass and framebuffer does not need to have depth.
     */
    mRenderDevice->createColorDepthRenderPass({}, false, mRenderPass);
    mRenderDevice->createDepthResources(mWindow->getWidth(), mWindow->getHeight(), mDepthImage);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, VK_NULL_HANDLE, mSwapchainFramebuffers);

    createDescriptors();

    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{vertexShaderFile, fragmentShaderFile},
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mGraphicsPipeline);

    return true;
}

void MultiMeshRenderer::updateIndirectBuffers(u32 swapchainImageIndex, bool* visibility)
{
    VkDrawIndirectCommand* data = nullptr;

    mRenderDevice->mapMemory(mIndirectBuffers[swapchainImageIndex].allocation, (void**)&data);

    for (u32 i = 0; i < mMaxDrawDataCount; i++)
    {
        const auto j = mDrawData[i].meshIndex;
        const auto lod = mDrawData[i].LOD;
        data[i] = {
            .vertexCount = mMeshData.meshes[j].getLODIndicesCount(lod),
            .instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
            .firstVertex = 0,
            .firstInstance = i,
        };
    }

    mRenderDevice->unmapMemory(mIndirectBuffers[swapchainImageIndex].allocation);
}

void MultiMeshRenderer::updateGeometryBuffers(u32 vertexSize, u32 indexSize, const void* vertices, const void* indices)
{

    mRenderDevice->uploadBufferData(mStorageBuffer, vertices, 0, vertexSize);
    mRenderDevice->uploadBufferData(mStorageBuffer, indices, mIndexBufferOffset, indexSize);
}

void MultiMeshRenderer::updateMaterialBuffer(u32 materialSize, const void* materialData)
{
    // nothing for now
}

void MultiMeshRenderer::updateUniformBuffer(const mat4& mat)
{
    UniformBuffer ubo = {
        .mvp = mat,
    };

    mRenderDevice->uploadBufferData(mUniformBuffers[mRenderDevice->getSwapchainImageIndex()], &ubo, sizeof(ubo));
}

void MultiMeshRenderer::updateDrawDataBuffer(u32 swapchainImageIndex, u32 drawDataSize, const void* drawData)
{
    mRenderDevice->uploadBufferData(mDrawDataBuffers[swapchainImageIndex], drawData, drawDataSize);
}

void MultiMeshRenderer::updateCountBuffer(u32 swapchainImageIndex, u32 itemCount)
{
    mRenderDevice->uploadBufferData(mDrawCountBuffers[swapchainImageIndex], &itemCount, sizeof(itemCount));
}

bool MultiMeshRenderer::createDescriptors()
{
    mRenderDevice->createDescriptorPool(1, 4, 0, mDescriptorPool);

    const std::vector<VkDescriptorSetLayoutBinding>
        bindings
        = {
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 4,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        };

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    u32 swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, mDescriptorSetLayout);

    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    // update the descriptor writes
    for (auto i = 0; i < swapchainImageCount; i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(UniformBuffer),
        };
        const VkDescriptorBufferInfo storageBufferVertexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = 0,
            .range = mMaxVertexBufferSize,
        };
        const VkDescriptorBufferInfo storageBufferIndexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = mMaxVertexBufferSize, // mIndexBufferOffset
            .range = mMaxIndexBufferSize,
        };
        const VkDescriptorBufferInfo storageBufferDrawDataInfo = {
            .buffer = mDrawDataBuffers[i].buffer,
            .offset = 0,
            .range = mMaxDrawDataSize,
        };
        const VkDescriptorBufferInfo storageBufferMaterialInfo = {
            .buffer = mMaterialBuffer.buffer,
            .offset = 0,
            .range = mMaxMaterialSize,
        };

        const std::vector<VkWriteDescriptorSet> descriptorWrites = {
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &uboInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferVertexInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferIndexInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferDrawDataInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 4,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferMaterialInfo},
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

} // namespace Suoh
