#include "CubeRenderer.h"

namespace Suoh
{

CubeRenderer::CubeRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage)
    : RendererBase(renderDevice, window, depthImage)
{
}

CubeRenderer::~CubeRenderer()
{
    if (mIsInitialized)
    {
        destroy();
    }
}

bool CubeRenderer::loadCubeMap(const std::string& filePath)
{
    // XXX: should reuse resources and only alter cube texture image, instead of destroying everthing
    if (mIsInitialized)
    {
        destroy();
    }

    mRenderDevice->createCubeTextureImage(mTexture.image, filePath);

    mRenderDevice->createImageView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, mTexture.image, VK_IMAGE_VIEW_TYPE_CUBE, 6);
    mRenderDevice->createTextureSampler(mTexture);

    createUniformBuffers(sizeof(UniformBuffer));

    mRenderDevice->createColorDepthRenderPass({}, true, mRenderPass);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);

    createDescriptorSets();
    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(),
                                          mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/Cube.vert",
                                              "../../../Suoh/Shaders/Cube.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mGraphicsPipeline);

    mIsInitialized = true;

    return true;
}

void CubeRenderer::destroy()
{
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

    for (auto& buffer : mUniformBuffers)
    {
        mRenderDevice->destroyBuffer(buffer);
    }
}

void CubeRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    if (!mIsInitialized)
        return;

    beginRenderPass(commandBuffer);

    vkCmdDraw(commandBuffer, 36, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
}

void CubeRenderer::updateUniformBuffer(const mat4& mvp)
{
    UniformBuffer ubo = {
        .mvp = mvp,
    };
    auto swapchainImageIndex = mRenderDevice->getSwapchainImageIndex();

    mRenderDevice->uploadBufferData(mUniformBuffers[swapchainImageIndex], &ubo, sizeof(ubo));
}

bool CubeRenderer::createDescriptorSets()
{
    mRenderDevice->createDescriptorPool(1, 0, 1, mDescriptorPool);

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
            .range = sizeof(UniformBuffer),
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
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &textureInfo,
            },
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

} // namespace Suoh