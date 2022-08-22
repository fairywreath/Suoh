#pragma once

#include <Core/SuohBase.h>
#include <Core/Types.h>

#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>

namespace Suoh
{

class RendererBase
{
public:
    virtual ~RendererBase() = default;

    SUOH_NON_COPYABLE(RendererBase);
    SUOH_NON_MOVEABLE(RendererBase);

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

} // namespace Suoh