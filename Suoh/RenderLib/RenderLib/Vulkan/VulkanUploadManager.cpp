#include "VulkanUploadManager.h"

#include "VulkanDevice.h"

namespace RenderLib
{

namespace Vulkan
{

UploadManager::~UploadManager()
{
    if (m_CurrentChunk)
    {
        if (m_CurrentChunk->mappedMemory)
        {
            m_pDevice->UnmapBuffer(m_CurrentChunk->buffer);
            m_CurrentChunk->mappedMemory = nullptr;
        }
    }
}

BufferChunkPtr UploadManager::GetChunk()
{
    if (!m_CurrentChunk)
    {
        // XXX: Hardcoded ~300 mb staging buffer.
        m_CurrentChunk = CreateChunk(268435456);
    }

    return m_CurrentChunk;
}

BufferChunkPtr UploadManager::CreateChunk(u64 size)
{
    BufferDesc desc{};
    desc.size = size;
    desc.cpuAccess = CpuAccessMode::WRITE;

    auto chunk = std::make_shared<BufferChunk>();
    chunk->buffer = m_pDevice->CreateBuffer(desc);
    chunk->size = desc.size;
    chunk->mappedMemory = m_pDevice->MapBuffer(chunk->buffer);

    return chunk;
}

} // namespace Vulkan

} // namespace RenderLib
