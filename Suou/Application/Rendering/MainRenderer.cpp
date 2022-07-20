#include "MainRenderer.h"

#include <array>

namespace Suou
{

MainRenderer::MainRenderer()
    : mRenderDevice(nullptr),
      mWindow(nullptr)
{
}

MainRenderer::~MainRenderer()
{
    destroy();
}

void MainRenderer::init(Window* window)
{
    mRenderDevice = std::make_unique<VKRenderDevice>(window);
    mWindow = window;
}

void MainRenderer::destroy()
{
    mRenderDevice->destroy();
    mRenderDevice.release();
}

void MainRenderer::render()
{
}

bool MainRenderer::fillCommandBuffer()
{
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };

    const std::array<VkClearValue, 2> clearValues = {
        VkClearValue{.color = ClearValueColor},
        VkClearValue{.depthStencil = {1.0f, 0}},
    };

    const VkRect2D screenRect = {
        .offset = {0, 0},
        .extent = {
            .width = mWindow->getWidth(),
            .height = mWindow->getHeight(),
        },
    };

    VkCommandBuffer commandBuffer = mRenderDevice->getCurrentCommandBuffer();
    size_t swapchainImageIdx = mRenderDevice->getSwapchainImageIndex();

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = mRenderPass,
        // XXX: get directly from swapchain?
        .framebuffer = mSwapchainFramebuffers[swapchainImageIdx],
        .clearValueCount = (u32)clearValues.size(),
        .pClearValues = clearValues.data(),
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                            &mDescriptorSets[swapchainImageIdx], 0, nullptr);

    vkCmdDraw(commandBuffer, (u32)(mIndexBufferSize / sizeof(u32)), 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return true;
}

} // namespace Suou