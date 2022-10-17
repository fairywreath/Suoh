#include "VulkanBuffer.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanBuffer::~VulkanBuffer()
{
    if (mappedMemory)
    {
        vmaUnmapMemory(m_Context.allocator, allocation);
        mappedMemory = nullptr;
    }

    for (auto&& p : viewCache)
    {
        m_Context.device.destroyBufferView(p.second);
    }
    viewCache.clear();

    if (managed)
    {
        assert(buffer != vk::Buffer());

        vmaDestroyBuffer(m_Context.allocator, buffer, allocation);
        buffer = vk::Buffer();
    }
}

Object VulkanBuffer::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_Buffer:
        return Object(buffer);
    default:
        return Object(nullptr);
    }
}

} // namespace Vulkan

} // namespace SuohRHI