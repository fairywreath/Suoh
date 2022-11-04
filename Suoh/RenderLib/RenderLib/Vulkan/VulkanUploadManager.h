#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"

#include <list>

namespace RenderLib
{

namespace Vulkan
{

struct BufferChunk
{
    BufferHandle buffer;
    void* mappedMemory{nullptr};
    u64 size;
};

using BufferChunkPtr = std::shared_ptr<BufferChunk>;

class VulkanDevice;

/*
 * Basically one large staging buffer for now.
 */
class UploadManager
{
public:
    explicit UploadManager(VulkanDevice* pDevice) : m_pDevice(pDevice)
    {
    }
    ~UploadManager();

    BufferChunkPtr GetChunk();

private:
    BufferChunkPtr CreateChunk(u64 size);

private:
    VulkanDevice* m_pDevice;

    BufferChunkPtr m_CurrentChunk;
};

} // namespace Vulkan

} // namespace RenderLib
