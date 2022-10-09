#include "RendererBase.h"

#include <Core/Logger.h>

#include <array>

namespace Suoh
{

RendererBase::RendererBase(VKRenderDevice* renderDevice, Window* window, const Image& depthImage)
    : mRenderDevice(renderDevice),
      mWindow(window),
      mDepthImage(depthImage)
{
}

bool RendererBase::createUniformBuffers(size_t uboSize)
{
    VkDeviceSize bufferSize = uboSize;

    mUniformBuffers.resize(mRenderDevice->getSwapchainImageCount());

    for (int i = 0; i < mUniformBuffers.size(); i++)
    {
        Buffer& buffer = mUniformBuffers[i];
        if (!mRenderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         //VMA_MEMORY_USAGE_GPU_ONLY,
                                         VMA_MEMORY_USAGE_CPU_ONLY, // need HOST_VISIBLE_BIT apparently
                                        buffer))
        {
            return false;
        }
    }

    return true;
}

void RendererBase::beginRenderPass(VkCommandBuffer commandBuffer)
{
    size_t swapchainImageIdx = mRenderDevice->getSwapchainImageIndex();

    const VkRect2D screenRect = {
        .offset = {0, 0},
        .extent = {
            .width = mWindow->getWidth(),
            .height = mWindow->getHeight(),
        },
    };

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = mRenderPass,
        .framebuffer = mSwapchainFramebuffers[swapchainImageIdx],
        .renderArea = screenRect,
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                            &mDescriptorSets[swapchainImageIdx], 0, nullptr);
}

} // namespace Suoh