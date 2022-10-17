#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"

#include <list>

namespace SuohRHI
{

namespace Vulkan
{

struct BufferChunk
{
    BufferHandle buffer;
    u64 bufferSize;
    u64 writePointer;

    void* mappedMemory{nullptr};

    // XXX: is this needed?
    u64 version{0};

    static constexpr u64 C_SIZE_ALIGNMENT{4096};
};

using BufferChunkPtr = std::shared_ptr<BufferChunk>;

class VulkanDevice;

class VulkanUploadManager
{
public:
    VulkanUploadManager(VulkanDevice* pDevice, u64 defaultChunkSize, u64 memoryLimit, bool isScratchBuffer)
        : m_pDevice(pDevice), m_DefaultChunkSize(defaultChunkSize), m_MemoryLimit(memoryLimit), m_IsScratchBuffer(isScratchBuffer)
    {
    }

    BufferChunkPtr CreateChunk(u64 size);

    bool SuballocateBuffer(u64 size, VulkanBuffer** pBuffer, u64* offset, void** pCpuVA, u64 currentVersion, u32 alignment = 256);
    void SubmitChunks(u64 currentVersion, u64 submittedVersion);

private:
    VulkanDevice* m_pDevice;

    u64 m_DefaultChunkSize{0};
    u64 m_MemoryLimit{0};

    // XXX: Is this needed?
    u64 m_AllocatedMemory{0};

    bool m_IsScratchBuffer{false};

    std::list<BufferChunkPtr> m_ChunkPool;
    BufferChunkPtr m_CurrentChunk;
};

} // namespace Vulkan

} // namespace SuohRHI