#include "VulkanBuffer.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include <Logger.h>

namespace RenderLib
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

BufferHandle VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    auto buffer = BufferHandle::Create(new Buffer(m_Context));

    vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eTransferSrc
                                       | vk::BufferUsageFlagBits::eTransferDst
                                       | ToVkBufferUsageFlags(desc.usage);

    auto bufferInfo = vk::BufferCreateInfo()
                          .setUsage(bufferUsage)
                          .setSize(desc.size)
                          .setSharingMode(vk::SharingMode::eExclusive)
                          .setPNext(0);

    VmaAllocationCreateInfo allocInfo{};
    if (desc.usage == BufferUsage::UNIFORM_BUFFER || desc.cpuAccess == CpuAccessMode::WRITE)
    {
        // Get OUT OF MEMORY if flags are used manually.
        // allocInfo.flags
        //     = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    }
    else
    {
        allocInfo.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    auto res = vmaCreateBuffer(
        m_Context.allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocInfo,
        reinterpret_cast<VkBuffer*>(&buffer->buffer), &buffer->allocation, nullptr);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create buffer! ", res);
        return nullptr;
    }

    buffer->desc = desc;

    return buffer;
}

} // namespace Vulkan

} // namespace RenderLib