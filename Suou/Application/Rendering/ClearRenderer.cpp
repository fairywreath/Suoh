#include "ClearRenderer.h"

#include <Logger.h>

namespace Suou
{

ClearRenderer::ClearRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage)
    : RendererBase(renderDevice, window, depthImage),
      mClearDepth(depthImage.image != VK_NULL_HANDLE)
{
    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = true,
            .clearDepth = true,
            .flags = eRenderPassBit::eRenderPassBitFirst,
        },
        mClearDepth, mRenderPass);

    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

ClearRenderer::~ClearRenderer()
{
    mRenderDevice->destroyRenderPass(mRenderPass);
    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }
}

void ClearRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    const VkClearValue clearValues[2] = {
        VkClearValue{
            .color = {1.0f, 1.0f, 1.0f, 1.0f},
        },
        VkClearValue{
            .depthStencil = {1.0f, 0},
        },
    };

    const VkRect2D screenRect = {
        .offset = {0, 0},
        .extent = {
            .width = mWindow->getWidth(),
            .height = mWindow->getHeight(),
        },
    };

    size_t swapchainImageIdx = mRenderDevice->getSwapchainImageIndex();

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mRenderPass,
        .framebuffer = mSwapchainFramebuffers[swapchainImageIdx],
        .renderArea = screenRect,
        .clearValueCount = static_cast<u32>(mClearDepth ? 2 : 1),
        .pClearValues = &clearValues[0],
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);
}

} // namespace Suou