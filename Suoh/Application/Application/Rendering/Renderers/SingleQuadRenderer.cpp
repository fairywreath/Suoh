#include "SingleQuadRenderer.h"

namespace Suoh
{

SingleQuadRenderer::SingleQuadRenderer(VKRenderDevice* renderDevice, Window* window)
    : RendererBase(renderDevice, window, Image())
{
}

SingleQuadRenderer::~SingleQuadRenderer()
{
    // XXX: should only destroy resources  when initialized
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
}

void SingleQuadRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
}

bool SingleQuadRenderer::init(const Texture& texture, VkImageLayout desiredLayout)
{
    createDescriptors(desiredLayout);

    mRenderDevice->createColorDepthRenderPass({}, false, mRenderPass);
    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/Quad.vert",
                                              "../../../Suoh/Shaders/Quad.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mGraphicsPipeline, false);

    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, VK_NULL_HANDLE, mSwapchainFramebuffers);

    mTexture = texture;

    return true;
}

bool SingleQuadRenderer::createDescriptors(VkImageLayout desiredLayout)
{
    mRenderDevice->createDescriptorPool(0, 0, 1, mDescriptorPool);

    const std::vector<VkDescriptorSetLayoutBinding>
        bindings
        = {
            VkDescriptorSetLayoutBinding{
                .binding = 0,
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
        const VkDescriptorImageInfo textureInfo = {
            .sampler = mTexture.sampler,
            .imageView = mTexture.image.imageView,
            .imageLayout = desiredLayout,
        };

        const std::vector<VkWriteDescriptorSet> descriptorWrites = {
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &textureInfo},
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

} // namespace Suoh
