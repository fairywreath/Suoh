#pragma once

#include <CoreTypes.h>
#include <SuouBase.h>

#include <RenderLib/Vulkan/VKRenderDevice.h>

namespace Suou
{

class RendererBase
{
public:
    virtual ~RendererBase() = default;

    SUOU_NON_COPYABLE(RendererBase);
    SUOU_NON_MOVEABLE(RendererBase);

    virtual void recordCommands(VkCommandBuffer commandBuffer) = 0;

    inline Image getDepthImage() const
    {
        return mDepthImage;
    };

protected:
    RendererBase(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);

    bool createUniformBuffers(size_t uboSize);
    void beginRenderPass(VkCommandBuffer commandBuffer);

protected:
    VKRenderDevice* mRenderDevice;
    Window* mWindow;

    VkRenderPass mRenderPass;
    std::vector<VkFramebuffer> mSwapchainFramebuffers; // have this inside swapchain itself?
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;

    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets; // 1 descriptor set per swapchain image for now

    std::vector<Buffer> mUniformBuffers;

    Image mDepthImage;
};

} // namespace Suou