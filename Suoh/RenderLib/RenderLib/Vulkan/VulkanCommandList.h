#pragma once

#include "VulkanCommon.h"

#include "VulkanBindings.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanUploadManager.h"

namespace RenderLib
{

namespace Vulkan
{

enum class CommandListUsage
{
    GRAPHICS,
    TRANSFER,
};

struct CommandListDesc
{
    QueueType queueType{QueueType::GRAPHICS};
    CommandListUsage usage{CommandListUsage::GRAPHICS};
};

struct GraphicsState
{
    FramebufferHandle frameBuffer{nullptr};
    RenderPassHandle renderPass{nullptr};
    GraphicsPipelineHandle pipeline{nullptr};

    DescriptorSetHandle descriptorSet{nullptr};

    BufferHandle indirectBuffer;
};

struct DrawArguments
{
    // Vertex or index count.
    u32 vertexCount{0};

    u32 instanceCount{1};
    u32 firstVertex{0};
    u32 firstInstance{0};

    i32 vertexOffset{0};
};

class CommandList : public RefCountResource<IResource>
{
public:
    CommandList(const VulkanContext& context, VulkanDevice* device, const CommandListDesc& desc);
    ~CommandList();

    void Begin();
    void End();

    void CopyBuffer(Buffer* src, Buffer* dst, u32 size, u32 srcOffset = 0, u32 dstOffset = 0);
    void CopyBufferToImage(Buffer* buffer, Image* image);

    void WriteBuffer(Buffer* buffer, const void* data, u32 size, u32 dstOffset = 0);
    void WriteImage(Image* image, const void* data);

    // Sets and begin graphics pipeline.
    void SetGraphicsState(const GraphicsState& graphicsState);

    void Draw(const DrawArguments& args);
    void DrawIndexed(const DrawArguments& args);
    void DrawIndirect(u32 offset, u32 drawCount);

    const vk::CommandBuffer& GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    CommandListDesc desc;

private:
    void InitCommandBuffer();

    void EndRenderPass();

    void TransitionImageLayout(Image* image, vk::ImageLayout newLayout);

private:
    const VulkanContext& m_Context;
    VulkanDevice* m_pDevice;

    vk::CommandBuffer m_CommandBuffer{nullptr};
    vk::CommandPool m_CommandPool{nullptr};

    UploadManager m_UploadManager;

    // Holds strong reference to graphics pipeline objects.
    GraphicsState m_CurrentGraphicsState{};
};

using CommandListHandle = RefCountPtr<CommandList>;

} // namespace Vulkan

} // namespace RenderLib
