#include "VKUploadBufferHandler.h"
#include "../VKRenderDevice.h"

#include <Asserts.h>

namespace Suou
{

static constexpr auto STAGING_BUFFER_SIZE = 16 * 1024 * 1024;

struct StagingBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VkDeviceSize size;
};

struct StagingBufferData : IStagingBufferData
{
    // use 1 global staging buffer for now
    StagingBuffer stagingBuffer;
};

static constexpr StagingBufferData& toStagingBufferData(IStagingBufferData* data)
{
    return static_cast<StagingBufferData&>(*data);
}

VKUploadBufferHandler::VKUploadBufferHandler(VKRenderDevice& renderDevice)
    : mRenderDevice(renderDevice), mData(std::make_unique<StagingBufferData>())
{
}

VKUploadBufferHandler::~VKUploadBufferHandler()
{
}

void VKUploadBufferHandler::init()
{
    initStagingBuffers();
}

void VKUploadBufferHandler::destroy()
{
    destroyStagingBuffers();
}

void VKUploadBufferHandler::initStagingBuffers()
{
    auto& data = toStagingBufferData(mData.get());

    VkBufferCreateInfo stagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = STAGING_BUFFER_SIZE,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };

    const VmaAllocationCreateInfo stagingBufferAllocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_ONLY,
    };

    StagingBuffer& stagingBuffer = data.stagingBuffer;
    VK_CHECK(vmaCreateBuffer(mRenderDevice.mAllocator, &stagingBufferInfo, &stagingBufferAllocInfo,
                             &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr));
    stagingBuffer.size = STAGING_BUFFER_SIZE;
}

void VKUploadBufferHandler::destroyStagingBuffers()
{
    auto& data = toStagingBufferData(mData.get());

    vmaDestroyBuffer(mRenderDevice.mAllocator, data.stagingBuffer.buffer, data.stagingBuffer.allocation);
}

UploadBuffer VKUploadBufferHandler::createUploadBuffer(BufferHandle targetBuffer, size_t size)
{
    // TODO: handle variable size
    if (size > STAGING_BUFFER_SIZE)
    {
        SUOU_ASSERT("Staging buffer is too small!");
    }

    // TODO: handle multithreaded case with multiple buffers

    auto& data = toStagingBufferData(mData.get());

    UploadBuffer uploadBuffer;
    uploadBuffer.size = STAGING_BUFFER_SIZE;
    vmaMapMemory(mRenderDevice.mAllocator, data.stagingBuffer.allocation, &uploadBuffer.mappedData);

    return uploadBuffer;
}

void VKUploadBufferHandler::destroyUploadBuffer(UploadBuffer& buffer)
{
    auto& data = toStagingBufferData(mData.get());
    vmaUnmapMemory(mRenderDevice.mAllocator, data.stagingBuffer.allocation);
}

void VKUploadBufferHandler::uploadToBuffer(BufferHandle dstBufferHandle, u64 dstOffset, const void* data, u64 srcOffset,
                                           u64 size)
{
    // auto& stagingData = toStagingBufferData(mData.get());

    // auto uploadBuffer = createUploadBuffer(dstBufferHandle, size);
    // memcpy(uploadBuffer.mappedData, &static_cast<const u8*>(data)[srcOffset], size);
    // destroyUploadBuffer(uploadBuffer);

    // // XXX: use external commandbuffer (uploadContext) ???
    // auto commandBuffer = mRenderDevice.beginSingleTimeCommands();

    // VkBuffer srcBuffer = stagingData.stagingBuffer.buffer;
    // VkBuffer dstBuffer = mRenderDevice.mBufferHandler.getVkBuffer(dstBufferHandle);
    // const VkBufferCopy copyRegion = {
    //     .srcOffset = 0, // srtOffset already set in memcpy above, stagingbuffer offset is currently 0
    //     .dstOffset = dstOffset,
    //     .size = size,
    // };

    // vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // mRenderDevice.endSingleTimeCommands(commandBuffer);
}

} // namespace Suou
