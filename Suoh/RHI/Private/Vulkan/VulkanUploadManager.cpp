#include "VulkanUploadManager.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include "RHIVersioning.h"

namespace SuohRHI
{

namespace Vulkan
{

BufferChunkPtr VulkanUploadManager::CreateChunk(u64 size)
{
    auto chunk = std::make_shared<BufferChunk>();

    if (m_IsScratchBuffer)
    {
        BufferDesc desc;
        desc.byteSize = size;
        desc.cpuAccess = CpuAccessMode::NONE;
        desc.supportStorageBuffer = true;

        // desc.usage = BufferUsage::STORAGE_BUFFER;

        chunk->buffer = m_pDevice->CreateBuffer(desc);
        chunk->bufferSize = size;
        chunk->mappedMemory = nullptr;
    }
    else
    {
        BufferDesc desc;
        desc.byteSize = size;
        desc.cpuAccess = CpuAccessMode::WRITE;

        chunk->buffer = m_pDevice->CreateBuffer(desc);
        chunk->bufferSize = size;
        chunk->mappedMemory = m_pDevice->MapBuffer(chunk->buffer, CpuAccessMode::WRITE);
    }

    return BufferChunkPtr();
}

bool VulkanUploadManager::SuballocateBuffer(u64 size, VulkanBuffer** pBuffer, u64* pOffset, void** pCpuVA, u64 currentVersion,
                                            u32 alignment)
{
    BufferChunkPtr retiredChunk;

    if (m_CurrentChunk)
    {
        u64 alignedOffset = align(m_CurrentChunk->writePointer, u64(alignment));
        u64 chunkEnd = alignedOffset + size;

        // If current chunk has empty bytes available.
        if (chunkEnd <= m_CurrentChunk->bufferSize)
        {
            m_CurrentChunk->writePointer = chunkEnd;

            *pBuffer = checked_cast<VulkanBuffer*>(m_CurrentChunk->buffer.Get());
            *pOffset = alignedOffset;

            // If memory is mapped.
            if (pCpuVA && m_CurrentChunk->mappedMemory)
                *pCpuVA = (char*)m_CurrentChunk->mappedMemory + alignedOffset;

            return true;
        }

        retiredChunk = m_CurrentChunk;
        m_CurrentChunk.reset();
    }

    auto queue = VersionGetQueue(currentVersion);
    u64 completedInstance = m_pDevice->QueueGetCompletedInstance(queue);

    for (auto it = m_ChunkPool.begin(); it != m_ChunkPool.end(); ++it)
    {
        BufferChunkPtr chunk = *it;

        if (VersionGetSubmitted(chunk->version) && VersionGetInstance(chunk->version) <= completedInstance)
        {
            chunk->version = 0;
        }

        if (chunk->version == 0 && chunk->bufferSize >= size)
        {
            m_ChunkPool.erase(it);
            m_CurrentChunk = chunk;
            break;
        }
    }

    if (retiredChunk)
    {
        m_ChunkPool.push_back(retiredChunk);
    }

    if (!m_CurrentChunk)
    {
        uint64_t sizeToAllocate = align(std::max(size, m_DefaultChunkSize), BufferChunk::C_SIZE_ALIGNMENT);

        if ((m_MemoryLimit > 0) && (m_AllocatedMemory + sizeToAllocate > m_MemoryLimit))
            return false;

        m_CurrentChunk = CreateChunk(sizeToAllocate);
    }

    m_CurrentChunk->version = currentVersion;
    m_CurrentChunk->writePointer = size;

    *pBuffer = checked_cast<VulkanBuffer*>(m_CurrentChunk->buffer.Get());
    *pOffset = 0;
    if (pCpuVA)
        *pCpuVA = m_CurrentChunk->mappedMemory;

    return true;
}

void VulkanUploadManager::SubmitChunks(u64 currentVersion, u64 submittedVersion)
{
    if (m_CurrentChunk)
    {
        m_ChunkPool.push_back(m_CurrentChunk);
        m_CurrentChunk.reset();
    }

    for (const auto& chunk : m_ChunkPool)
    {
        if (chunk->version == currentVersion)
            chunk->version = submittedVersion;
    }
}

} // namespace Vulkan

} // namespace SuohRHI