#pragma once

#include "VulkanCommon.h"

namespace RenderCore
{

namespace Vulkan
{

struct BufferDesc
{
    u32 size;
    BufferUsage usage;
    CpuAccessMode cpuAccess{CpuAccessMode::NONE};
};

class Buffer : public RefCountResource<IResource>
{
public:
    explicit Buffer(const VulkanContext& context) : m_Context(context)
    {
    }
    ~Buffer() override;

    BufferDesc desc;

    vk::Buffer buffer;
    VmaAllocation allocation;

private:
    VulkanContext m_Context;
};

using BufferHandle = RefCountPtr<Buffer>;

} // namespace Vulkan

} // namespace RenderCore
