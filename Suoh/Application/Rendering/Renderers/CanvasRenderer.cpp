#include "CanvasRenderer.h"

namespace Suoh
{

CanvasRenderer::CanvasRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage)
    : RendererBase(renderDevice, window, depthImage)
{
    const auto imageCount = mRenderDevice->getSwapchainImageCount();
    mStorageBuffers.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++)
    {
        mRenderDevice->createBuffer(MAX_LINES_DATA_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, mStorageBuffers[i]);
    }

    createUniformBuffers(sizeof(UniformBuffer));
    createDescriptors();

    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = false,
            .clearDepth = false,
            .flags = 0,
        },
        (mDepthImage.image != VK_NULL_HANDLE), mRenderPass);
    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/Lines.vert",
                                              "../../../Suoh/Shaders/Lines.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                          mGraphicsPipeline);

    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

CanvasRenderer::~CanvasRenderer()
{
    for (auto& buffer : mStorageBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& buffer : mUniformBuffers)
        mRenderDevice->destroyBuffer(buffer);

    for (auto& frameBuffer : mSwapchainFramebuffers)
        mRenderDevice->destroyFramebuffer(frameBuffer);

    mRenderDevice->destroyRenderPass(mRenderPass);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyPipeline(mGraphicsPipeline);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);
}

void CanvasRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    if (mLines.empty())
        return;

    beginRenderPass(commandBuffer);

    vkCmdDraw(commandBuffer, static_cast<u32>(mLines.size()), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void CanvasRenderer::clear()
{
    mLines.clear();
}

void CanvasRenderer::drawLine(const vec3& p1, const vec3& p2, const vec4& color)
{
    mLines.push_back({
        .position = p1,
        .color = color,
    });
    mLines.push_back({
        .position = p2,
        .color = color,
    });
}

void CanvasRenderer::drawPlane(const vec3& o, const vec3& v1, const vec3& v2, int n1, int n2, float s1, float s2, const vec4& color, const vec4& outlineColor)
{
    drawLine(o - s1 / 2.0f * v1 - s2 / 2.0f * v2, o - s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
    drawLine(o + s1 / 2.0f * v1 - s2 / 2.0f * v2, o + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);

    drawLine(o - s1 / 2.0f * v1 + s2 / 2.0f * v2, o + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
    drawLine(o - s1 / 2.0f * v1 - s2 / 2.0f * v2, o + s1 / 2.0f * v1 - s2 / 2.0f * v2, outlineColor);

    for (int i = 1; i < n1; i++)
    {
        float t = ((float)i - (float)n1 / 2.0f) * s1 / (float)n1;
        const vec3 o1 = o + t * v1;
        drawLine(o1 - s2 / 2.0f * v2, o1 + s2 / 2.0f * v2, color);
    }

    for (int i = 1; i < n2; i++)
    {
        const float t = ((float)i - (float)n2 / 2.0f) * s2 / (float)n2;
        const vec3 o2 = o + t * v2;
        drawLine(o2 - s1 / 2.0f * v1, o2 + s1 / 2.0f * v1, color);
    }
}

void CanvasRenderer::updateBuffer()
{
    if (mLines.empty())
        return;

    const size_t bufferSize = mLines.size() * sizeof(VertexData);

    mRenderDevice->uploadBufferData(mStorageBuffers[mRenderDevice->getSwapchainImageIndex()], mLines.data(), bufferSize);
}

void CanvasRenderer::updateUniformBuffer(const mat4& mvp, float time)
{
    const UniformBuffer ubo = {
        .mvp = mvp,
        .time = time,
    };

    mRenderDevice->uploadBufferData(mUniformBuffers[mRenderDevice->getSwapchainImageIndex()], &ubo, sizeof(ubo));
}

void CanvasRenderer::createDescriptors()
{
    // create the descriptor sets
    mRenderDevice->createDescriptorPool(1, 1, 0, mDescriptorPool);

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
        };

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    u32 swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, mDescriptorSetLayout);

    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    // update the descriptor writes
    for (size_t i = 0; i < mRenderDevice->getSwapchainImageCount(); i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(UniformBuffer),
        };
        const VkDescriptorBufferInfo sboInfo = {
            .buffer = mStorageBuffers[i].buffer,
            .offset = 0,
            .range = MAX_LINES_DATA_SIZE,
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
                .pBufferInfo = &sboInfo,
            },
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }
}

} // namespace Suoh