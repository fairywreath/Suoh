#pragma once

#include <RenderLib/Vulkan/VKRenderDevice.h>

namespace Suou
{

class MainRenderer
{
public:
    MainRenderer();
    virtual ~MainRenderer();

    void init(Window* window);
    void destroy();

    void render();

private:
    bool fillCommandBuffer();

private:
    static constexpr VkClearColorValue ClearValueColor = {1.0f, 1.0f, 1.0f, 1.0f};

private:
    std::unique_ptr<VKRenderDevice> mRenderDevice;
    Window* mWindow;

    // 1. No command buffer object yet, used pre-allocated commandbuffer from render device

    // 2. Graphics pipeline and render pass
    VkRenderPass mRenderPass;
    std::vector<VkFramebuffer> mSwapchainFramebuffers; // have this inside swapchain itself?
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;

    // 3. Descriptors sets and layout
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    std::vector<VkDescriptorSet> mDescriptorSets;

    // misc mesh/model data
    size_t mVertexBufferSize;
    size_t mIndexBufferSize;
};

} // namespace Suou