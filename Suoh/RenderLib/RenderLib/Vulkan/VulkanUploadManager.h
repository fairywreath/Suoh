#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"

#include <list>

namespace RenderLib
{

namespace Vulkan
{

static constexpr auto C_DEFAULT_UPLOAD_CHUNK_SIZE = 4096;

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
    UploadManager(VulkanDevice* pDevice, u64 defaultChunkSize = C_DEFAULT_UPLOAD_CHUNK_SIZE)
        : m_pDevice(pDevice), m_DefaultChunkSize(defaultChunkSize)
    {
    }
    ~UploadManager();

    BufferChunkPtr GetChunk();

private:
    BufferChunkPtr CreateChunk(u64 size);

private:
    VulkanDevice* m_pDevice;

    BufferChunkPtr m_CurrentChunk;

    u64 m_DefaultChunkSize{0};
};

} // namespace Vulkan

} // namespace RenderLib
