#include "VulkanCommon.h"
#include "VulkanDevice.h"

#include <Core/Logger.h>

Buffer::Buffer(VulkanDevice* device, const BufferDesc& desc) : m_Device(std::move(device)), m_Desc(desc)
{
    static constexpr vk::BufferUsageFlags dynamicBufferMask = vk::BufferUsageFlagBits::eVertexBuffer
                                                              | vk::BufferUsageFlagBits::eIndexBuffer
                                                              | vk::BufferUsageFlagBits::eUniformBuffer;

    const bool useGlobalBuffer = (desc.bufferUsage & dynamicBufferMask) != vk::BufferUsageFlagBits(0);

    if ((desc.resourceUsage == ResourceUsageType::Dynamic) && useGlobalBuffer)
    {
        m_ParentBuffer = m_Device->GetDynamicBuffer();
        return;
    }

    auto bufferInfo = vk::BufferCreateInfo()
                          .setUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst
                                    | desc.bufferUsage)
                          .setSize(desc.size);

    assert(desc.size > 0);
    assert(!(desc.deviceOnly && desc.persistent));

    VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT,
    };

    if (desc.persistent)
    {
        allocationCreateInfo.flags = allocationCreateInfo.flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    // XXX: These vma flags are deprecated. Use something else?
    if (desc.deviceOnly)
    {
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    else
    {
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
    }

    const auto vmaAllocator = m_Device->GetVmaAllocator();
    VmaAllocationInfo allocationInfo{};

    auto res = vmaCreateBuffer(vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocationCreateInfo,
                               reinterpret_cast<VkBuffer*>(&m_VulkanBuffer), &m_VmaAllocation, &allocationInfo);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create buffer!");
        return;
    }

#if defined(_DEBUG)
    vmaSetAllocationName(vmaAllocator, m_VmaAllocation, desc.name.c_str());
#endif

    m_Device->SetVulkanResourceName(vk::ObjectType::eBuffer, m_VulkanBuffer, desc.name);

    if (desc.initialData)
    {
        assert(!desc.deviceOnly);
        void* data;
        vmaMapMemory(vmaAllocator, m_VmaAllocation, &data);
        memcpy(data, desc.initialData, (size_t)desc.size);
        vmaUnmapMemory(vmaAllocator, m_VmaAllocation);
    }

    if (desc.persistent)
    {
        m_MappedData = static_cast<u8*>(allocationInfo.pMappedData);
    }

    m_Name = desc.name;
    m_Size = desc.size;
    m_BufferUsageFlags = desc.bufferUsage;
    m_ResourceUsage = desc.resourceUsage;

    m_Desc = desc;
}

Buffer::~Buffer()
{
    if (m_VulkanBuffer)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueAllocatedResource(DeferredDeletionQueue::Type::BufferAllocated,
                                                                      m_VulkanBuffer, m_VmaAllocation);
    }
}

BufferRef VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    auto bufferRef = BufferRef::Create(new Buffer(this, desc));

    if (!bufferRef->GetVulkanHandle())
    {
        bufferRef = nullptr;
    }

    return bufferRef;
}