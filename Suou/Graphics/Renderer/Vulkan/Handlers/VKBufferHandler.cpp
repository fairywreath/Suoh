#include "VKBufferHandler.h"
#include "../VKRenderDevice.h"
#include "../VKUtils.h"

#include <queue>
#include <vector>

#include <vk_mem_alloc.h>

#include <Asserts.h>

namespace Suou
{

struct Buffer
{
    VkDeviceSize size;
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct VKBufferHandlerData : IVKBufferHandlerData
{
    // XXX: handle multithreaded case, use concurrent vec
    std::vector<Buffer> buffers;
    std::queue<BufferHandle> returnedBufferHandles;
};

static inline VKBufferHandlerData& toBufferHandlerData(IVKBufferHandlerData* data)
{
    return static_cast<VKBufferHandlerData&>(*data);
}

VKBufferHandler::VKBufferHandler(VKRenderDevice& renderDevice)
    : mRenderDevice(renderDevice), mData(std::make_unique<VKBufferHandlerData>())
{
}

VKBufferHandler::~VKBufferHandler()
{
}

void VKBufferHandler::destroy()
{
}

BufferHandle VKBufferHandler::createBuffer(const BufferDescription& desc)
{
    auto device = mRenderDevice.mDevice.getLogical();
    auto& data = toBufferHandlerData(mData.get());

    BufferHandle handle = acquireNewHandle();
    Buffer& buffer = data.buffers[toHandleType(handle)];
    SUOU_ASSERT(buffer.size == 0);

    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = desc.size,
        .usage = VKUtils::toVkBufferUsageFlags(desc.usage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    const VmaAllocationCreateInfo vmaallocInfo{.usage = VKUtils::toVmaMemoryUsage(desc.memoryUsage)};

    VK_CHECK(vmaCreateBuffer(mRenderDevice.mAllocator, &bufferInfo, &vmaallocInfo, &buffer.buffer, &buffer.allocation,
                             nullptr));
    buffer.size = desc.size;

    return handle;
}

void VKBufferHandler::destroyBuffer(BufferHandle handle)
{
    HandleType handleValue = toHandleType(handle);
    auto& data = toBufferHandlerData(mData.get());

    SUOU_ASSERT(handleValue < data.buffers.size());

    Buffer& buffer = data.buffers[handleValue];

    vmaDestroyBuffer(mRenderDevice.mAllocator, buffer.buffer, buffer.allocation);
    buffer.size = 0;

    data.returnedBufferHandles.push(handle);
}

BufferHandle VKBufferHandler::acquireNewHandle()
{
    BufferHandle handle;
    auto& data = toBufferHandlerData(mData.get());

    if (!data.returnedBufferHandles.empty())
    {
        handle = data.returnedBufferHandles.front();
        data.returnedBufferHandles.pop();
    }
    else
    {
        handle = BufferHandle(static_cast<HandleType>(data.buffers.size()));
        data.buffers.emplace_back();
    }

    return handle;
}

VkBuffer VKBufferHandler::getVkBuffer(BufferHandle handle) const
{
    HandleType handleValue = toHandleType(handle);
    auto& data = toBufferHandlerData(mData.get());

    SUOU_ASSERT(handleValue < data.buffers.size());

    Buffer& buffer = data.buffers[handleValue];
    SUOU_ASSERT(buffer.size != 0);

    return buffer.buffer;
}

} // namespace Suou
