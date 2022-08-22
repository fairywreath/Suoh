#include "FinishRenderer.h"

namespace Suoh
{

FinishRenderer::FinishRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage)
    : RendererBase(renderDevice, window, depthImage)
{
    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = false,
            .clearDepth = false,
            .flags = eRenderPassBit::eRenderPassBitLast,
        },
        (mDepthImage.image != VK_NULL_HANDLE), mRenderPass);

    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

FinishRenderer::~FinishRenderer()
{
    mRenderDevice->destroyRenderPass(mRenderPass);
    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }
}

void FinishRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
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
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);
}

} // namespace Suoh