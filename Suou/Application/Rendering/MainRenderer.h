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
    static constexpr VkClearColorValue ClearValueColor = {1.0f, 1.0f, 1.0f, 1.0f};

    struct UniformBuffer
    {
        mat4 mvp;
    } ubo;

    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    struct AllocatedImage
    {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
    };

    struct AllocatedTexture
    {
        AllocatedImage image;
        VkSampler sampler;
    };

private:
    bool fillCommandBuffer();

    bool createUniformBuffers();
    void updateUniformBuffer(u32 imageIndex, const UniformBuffer& ubo);

    bool createDescriptors();
    void updateDescriptors();

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
    std::vector<VkDescriptorSet> mDescriptorSets; // 1 descriptor set per swapchain image for now

    // 4. UBO to store mvp matrix
    std::vector<Buffer> mUniformBuffers;
    Buffer mStorageBuffer;

    // 5. (color) Texture
    Texture mTexture;

    // 6. Depth buffer
    Image mDepthImage;

    // misc mesh/model data
    size_t mVertexBufferSize;
    size_t mIndexBufferOffset; // offset of index buffer in SSBO
    size_t mIndexBufferSize;
};

} // namespace Suou