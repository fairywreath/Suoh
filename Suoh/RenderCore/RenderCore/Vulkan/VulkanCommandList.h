#pragma once

#include "VulkanCommon.h"

#include "VulkanImage.h"
#include "VulkanUploadManager.h"

namespace RenderCore
{

namespace Vulkan
{

struct CommandListDesc
{
    QueueType queueType{QueueType::GRAPHICS};
};

class CommandList : public RefCountResource<IResource>
{
public:
    CommandList(const VulkanContext& context, VulkanDevice* device, const CommandListDesc& desc);
    ~CommandList();

    void CopyBuffer(Buffer* src, Buffer* dst, u32 size, u32 srcOffset = 0, u32 dstOffset = 0);
    void CopyBufferToImage(Buffer* buffer, Image* image);

    void WriteBuffer(Buffer* buffer, const void* data, u32 size, u32 dstOffset = 0);
    void WriteImage(Image* image, const void* data);

    vk::CommandBuffer GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    CommandListDesc desc;

private:
    void InitCommandBuffer();

    void TransitionImageLayout(Image* image, vk::ImageLayout newLayout);

private:
    const VulkanContext& m_Context;
    VulkanDevice* m_pDevice;

    vk::CommandBuffer m_CommandBuffer{nullptr};
    vk::CommandPool m_CommandPool{nullptr};

    UploadManager m_UploadManager;
};

using CommandListHandle = RefCountPtr<CommandList>;

} // namespace Vulkan

} // namespace RenderCore
