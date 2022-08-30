#include "QuadRenderer.h"

namespace Suoh
{

static constexpr auto MAX_QUADS = 256;

QuadRenderer::QuadRenderer(VKRenderDevice* renderDevice, Window* window, const std::vector<std::string>& textureFiles)
    : RendererBase(renderDevice, window, Image())
{
    loadTextureFiles(textureFiles);
}

QuadRenderer::~QuadRenderer()
{
    mRenderDevice->destroyPipeline(mGraphicsPipeline);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyRenderPass(mRenderPass);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);

    // mRenderDevice->destroyImage(mDepthImage);

    for (auto& frameBuffer : mSwapchainFramebuffers)
        mRenderDevice->destroyFramebuffer(frameBuffer);

    for (auto& buffer : mUniformBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& buffer : mStorageBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& texture : mTextures)
        mRenderDevice->destroyTexture(texture);
}

void QuadRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    if (mQuads.empty())
        return;

    beginRenderPass(commandBuffer);

    vkCmdDraw(commandBuffer, static_cast<u32>(mQuads.size()), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void QuadRenderer::loadTextureFiles(const std::vector<std::string>& textureFiles)
{
    const auto swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    mStorageBuffers.resize(swapchainImageCount);

    mVertexBufferSize = MAX_QUADS * 6 * sizeof(VertexData);

    for (auto i = 0; i < swapchainImageCount; i++)
    {
        mRenderDevice->createBuffer(mVertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    VMA_MEMORY_USAGE_CPU_ONLY, mStorageBuffers[i]);
    }

    // uniform buffers array will be used as push constant buffers
    createUniformBuffers(sizeof(PushConstantBuffer));

    const auto numTextureFiles = textureFiles.size();
    mTextures.resize(numTextureFiles);

    for (auto i = 0; i < numTextureFiles; i++)
    {
        mRenderDevice->createTextureImage(textureFiles[i], mTextures[i]);
        mRenderDevice->createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mTextures[i].image);
        mRenderDevice->createTextureSampler(mTextures[i]);
    }

    // mRenderDevice->createDepthResources(mWindow->getWidth(), mWindow->getHeight(), mDepthImage);

    createDescriptors();

    mRenderDevice->createColorDepthRenderPass({}, false, mRenderPass);
    mRenderDevice->createPipelineLayoutWithConstants(mDescriptorSetLayout, mPipelineLayout, sizeof(PushConstantBuffer), 0);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/TextureArray.vert",
                                              "../../../Suoh/Shaders/TextureArray.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mGraphicsPipeline);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, VK_NULL_HANDLE /*mDepthImage.imageView*/, mSwapchainFramebuffers);
}

void QuadRenderer::updateBuffer(u32 i)
{
    mRenderDevice->uploadBufferData(mStorageBuffers[i], mQuads.data(), sizeof(VertexData) * mQuads.size());
}

void QuadRenderer::pushConstants(VkCommandBuffer commandBuffer, u32 textureIndex, const vec2& offset)
{
    const PushConstantBuffer constBuffer = {
        .offset = offset,
        .textureIndex = textureIndex,
    };

    vkCmdPushConstants(commandBuffer, mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantBuffer), &constBuffer);
}

void QuadRenderer::quad(float x1, float y1, float x2, float y2)
{
    VertexData v1{{x1, y1, 0}, {0, 0}};
    VertexData v2{{x2, y1, 0}, {1, 0}};
    VertexData v3{{x2, y2, 0}, {1, 1}};
    VertexData v4{{x1, y2, 0}, {0, 1}};

    mQuads.push_back(v1);
    mQuads.push_back(v2);
    mQuads.push_back(v3);

    mQuads.push_back(v1);
    mQuads.push_back(v3);
    mQuads.push_back(v4);
}

void QuadRenderer::clear()
{
    mQuads.clear();
}

bool QuadRenderer::createDescriptors()
{
    mRenderDevice->createDescriptorPool(1, 1, 1, mDescriptorPool);

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
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = (u32)mTextures.size(),
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        };

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    u32 swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, mDescriptorSetLayout);

    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    std::vector<VkDescriptorImageInfo> textureDescriptors(mTextures.size());
    for (auto i = 0; i < mTextures.size(); i++)
    {
        textureDescriptors[i] = {
            .sampler = mTextures[i].sampler,
            .imageView = mTextures[i].image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    // update the descriptor writes
    for (auto i = 0; i < mRenderDevice->getSwapchainImageCount(); i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(PushConstantBuffer),
        };
        const VkDescriptorBufferInfo storageBufferVertexInfo = {
            .buffer = mStorageBuffers[i].buffer,
            .offset = 0,
            .range = mVertexBufferSize,
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
                .descriptorCount = (u32)textureDescriptors.size(),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = textureDescriptors.data(),
            },
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

} // namespace Suoh