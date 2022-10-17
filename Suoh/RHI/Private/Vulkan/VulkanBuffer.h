#pragma once

#include "VulkanCommon.h"

#include "CommandListStateTracker.h"

#include <vk_mem_alloc.h>

#include <unordered_map>

namespace SuohRHI
{

namespace Vulkan
{

/*
 * XXX: Need a volatile buffer implementation? maybe base it on nvrhi.
 */

// Copyable and assignable std::atomic
class BufferVersionCounter : public std::atomic<u64>
{
public:
    BufferVersionCounter() : std::atomic<u64>()
    {
    }

    BufferVersionCounter(const BufferVersionCounter& other)
    {
        store(other);
    }
    BufferVersionCounter& operator=(const u64 a)
    {
        store(a);
        return *this;
    }
};

class VulkanBuffer : public RefCountResource<IBuffer>, public BufferStateTracker
{
public:
    explicit VulkanBuffer(const VulkanContext& context) : BufferStateTracker(desc), m_Context(context)
    {
    }
    ~VulkanBuffer() override;

    const BufferDesc& GetDesc() const override
    {
        return desc;
    }
    Object GetNativeObject(ObjectType type) override;

    BufferDesc desc;

    vk::Buffer buffer;
    VmaAllocation allocation;

    std::unordered_map<vk::Format, vk::BufferView> viewCache;

    bool managed{true};

    // XXX: Are these needed?
    std::vector<BufferVersionCounter> versionTracking;
    void* mappedMemory{nullptr};
    u32 versionSearchStart{0};

    // For staging buffers.
    CommandQueue lastUseQueue{CommandQueue::GRAPHICS};
    u64 lsatUseCommandListID{0};

private:
    const VulkanContext& m_Context;
};

} // namespace Vulkan

} // namespace SuohRHI