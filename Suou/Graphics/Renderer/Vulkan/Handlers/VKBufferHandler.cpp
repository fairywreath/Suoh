#include "VKBufferHandler.h"
#include "../VKRenderDevice.h"

#include <vector>
#include <queue>

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

VKBufferHandler::VKBufferHandler(VKRenderDevice& renderDevice) :
    mRenderDevice(renderDevice),
    mData(nullptr)
{    
    mData = std::make_unique<VKBufferHandlerData>();
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

    const VkBufferCreateInfo bufferInfo = 
    {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = desc.size,

        // TODO
        // .usage = desc.usage,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,

		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

    const VmaAllocationCreateInfo vmaallocInfo 
    {
        // TODO
        // .usage = desc.memoryUsage
        .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    BufferHandle handle = acquireNewHandle();
    Buffer& buffer = data.buffers[toHandleType(handle)];
    SU_ASSERT(buffer.size == 0);
    VK_CHECK(vmaCreateBuffer(mRenderDevice.mAllocator, &bufferInfo, &vmaallocInfo,
        &buffer.buffer,
        &buffer.allocation,
        nullptr));
    buffer.size = desc.size;

    return handle;
}

void VKBufferHandler::destroyBuffer(BufferHandle handle)
{
    HandleType handleValue = toHandleType(handle);
    auto& data = toBufferHandlerData(mData.get());

    SU_ASSERT(handleValue < data.buffers.size());

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
        handle = BufferHandle(static_cast<type_safe::underlying_type<BufferHandle>>(data.buffers.size()));
        data.buffers.emplace_back();
    }

    return handle;
}

}