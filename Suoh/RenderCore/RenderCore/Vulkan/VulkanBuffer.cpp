#include "VulkanBuffer.h"

#include "Logger.h"

namespace RenderCore
{

namespace Vulkan
{

Buffer::~Buffer()
{
    if (buffer)
    {
        vmaDestroyBuffer(m_Context.allocator, buffer, allocation);
    }
}

} // namespace Vulkan

} // namespace RenderCore