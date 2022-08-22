#include "ModelRenderer.h"

namespace Suoh
{

ModelRenderer::ModelRenderer(VKRenderDevice* renderDevice, Window* window)
    : RendererBase(renderDevice, window, Image())
{
}

ModelRenderer::~ModelRenderer()
{
    mRenderDevice->destroyBuffer(mStorageBuffer);
    mRenderDevice->destroyTexture(mTexture);

    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }

    mRenderDevice->destroyRenderPass(mRenderPass);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyPipeline(mGraphicsPipeline);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);

    mRenderDevice->destroyImage(mDepthImage);

    for (auto& buffer : mUniformBuffers)
    {
        mRenderDevice->destroyBuffer(buffer);
    }
}

void ModelRenderer::loadModel(const std::string& modelFile, const std::string& textureFile, size_t uniformDataSize)
{
    // initialize vertex buffer
    mRenderDevice->createTexturedVertexBuffer(modelFile, mStorageBuffer, mVertexBufferSize, mIndexBufferSize, mIndexBufferOffset);
    createUniformBuffers(uniformDataSize);

    // initialize texture
    mRenderDevice->createTextureImage(textureFile, mTexture);
    mRenderDevice->createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mTexture.image);
    mRenderDevice->createTextureSampler(mTexture);

    // initialize depth buffer
    mRenderDevice->createDepthResources(mWindow->getWidth(), mWindow->getHeight(), mDepthImage);

    // initialize descriptor set
    createDescriptors(uniformDataSize);

    // initialize graphics pipeline
    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = false,
            .clearDepth = false,
            .flags = 0,
        },
        true, mRenderPass);
    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/Simple.vert",
                                              "../../../Suoh/Shaders/Simple.geom",
                                              "../../../Suoh/Shaders/Simple.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                          mGraphicsPipeline);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

void ModelRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    beginRenderPass(commandBuffer);
    vkCmdDraw(commandBuffer, (u32)(mIndexBufferSize / sizeof(u32)), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}
void ModelRenderer::updateUniformBuffer(const void* data, size_t uniformDataSize)
{
    mRenderDevice->uploadBufferData(mUniformBuffers[mRenderDevice->getSwapchainImageIndex()], data, uniformDataSize);
}

void ModelRenderer::createDescriptors(size_t uniformDataSize)
{
    // create the descriptor sets
    mRenderDevice->createDescriptorPool(1, 2, 1, mDescriptorPool);

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
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
    for (auto i = 0; i < mRenderDevice->getSwapchainImageCount(); i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = uniformDataSize,
        };
        const VkDescriptorBufferInfo storageBufferVertexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = 0,
            .range = mVertexBufferSize,
        };
        const VkDescriptorBufferInfo storageBufferIndexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = mIndexBufferOffset,
            .range = mIndexBufferSize,
        };
        const VkDescriptorImageInfo textureInfo = {
            .sampler = mTexture.sampler,
            .imageView = mTexture.image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &textureInfo},
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }
}

} // namespace Suoh