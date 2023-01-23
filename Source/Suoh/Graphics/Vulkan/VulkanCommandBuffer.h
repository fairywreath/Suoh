#pragma once

#include "GPUProfiler.h"
#include "VulkanResources.h"

class CommandBufferManager;

class CommandBuffer
{
public:
    CommandBuffer() = default;
    ~CommandBuffer() = default;
    // NON_COPYABLE(CommandBuffer);
    // NON_MOVEABLE(CommandBuffer);

    void Init(VulkanDevice* device);
    void Destroy();

    void Reset();

    // DescriptorSetRef CreateDescriptorSet(const DescriptorSetDesc& desc);

    void Begin();
    void BeginSecondary(RenderPass* currentRenderPass, Framebuffer* currentFramebuffer);

    void End();
    void EndCurrentRenderPass();

    void BindRenderPass(RenderPass* renderPass, Framebuffer* framebuffer, bool useSecondary = false);
    void BindPipeline(Pipeline* pipeline);

    // void BindVertexBuffer(Buffer* buffer, u32 binding, u32 offset);
    // void BindVertexBuffers(const std::vector<Buffer*>& buffers, u32 firstBinding, u32
    // bindingCount,
    //                        const std::vector<u32>& offsets);
    // void BindIndexBuffer(Buffer buffer, u32 offset, vk::IndexType indexType);

    void BindDescriptorSets(const std::vector<DescriptorSet*>& descriptorSets);
    // void BindLocalDescriptorSets(const std::vector<DescriptorSet*>& descriptorSets);

    void SetViewport(const Viewport& viewport);
    void SetScissor(const Rect2DInt& rect);

    void PushConstants(Pipeline* pipeline, u32 offset, u32 size, void* data);

    void Clear(f32 red, f32 green, f32 blue, f32 alpha, u32 attachmentIndex);
    void ClearDepthStencil(f32 depth, u8 stencil);

    void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);
    void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance);
    void DrawIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride);
    void DrawIndirectCount(Buffer* argumentBuffer, u32 argumentOffset, Buffer* countBuffer, u32 countOffset,
                           u32 maxDraws, u32 stride);
    void DrawIndexedIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride);

    void DrawMeshTask(u32 taskCount, u32 firstTask);
    void DrawMeshTaskIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride);
    void DrawMeshTaskIndirectCount(Buffer* argumentBuffer, u32 argumentOffset, Buffer* countBuffer, u32 countOffset,
                                   u32 maxDraws, u32 stride);

    void Dispatch(u32 groupX, u32 groupY, u32 groupZ);
    void DispatchIndirect(Buffer* buffer, u32 offset);

    // void Barrier(const ExecutionBarrier& barrier);
    void BufferBarrier(Buffer* buffer, ResourceState oldState, ResourceState new_State, QueueType srcQueueType,
                       QueueType dstQueueType);
    void TextureBarrier(Texture* texture, ResourceState newState, u32 mipLevel, u32 mipCount);

    // void FillBuffer(Buffer* buffer, u32 offset, u32 size, u32 data);

    // void PushMarker(const std::string& name);
    // void PopMarker();

    /*
     * Non drawing commands.
     */
    void CopyBuffer(Buffer* src, Buffer* dst, u32 size, u32 srcOffset = 0, u32 dstOffset = 0);

    // XXX: Handle subresource/layers/mips.
    void CopyBufferToTexture(Buffer* src, Texture* dst);

    void CopyTexture(Texture* src, Texture* dst, ResourceState dstState);

    void UploadBufferData(Buffer* buffer, const void* bufferData, u32 size, Buffer* stagingBuffer,
                          size_t stagingBufferOffset);
    void UploadTextureData(Texture* texture, const void* textureData, Buffer* stagingBuffer,
                           size_t stagingBufferOffset);

    // void SetIsRecording(bool value)
    // {
    //     m_IsRecording = value;
    // }

    void SetCurrentRenderPass(RenderPass* renderPass)
    {
        m_CurrentRenderPass = renderPass;
    }

    vk::CommandBuffer GetVulkanCommandBuffer() const
    {
        return m_VulkanCommandBuffer;
    }

    GPUThreadFramePools* GetThreadFramePools()
    {
        return m_ThreadFramePools;
    }

    u32 GetIndex() const
    {
        return m_Index;
    }

private:
    VulkanDevice* m_Device{nullptr};

    vk::CommandBuffer m_VulkanCommandBuffer;
    vk::DescriptorPool m_VulkanDescriptorPool;

    GPUThreadFramePools* m_ThreadFramePools{nullptr};

    std::vector<DescriptorSet*> m_DescriptorSets;
    // std::vector<DescriptorSetRef> m_DescriptorSets;

    std::vector<vk::DescriptorSet> m_VulkanDescriptorSets;
    // std::array<vk::DescriptorSet, K_MaxCommandBufferDescriptorSets> m_VulkanDescriptorSets;
    // u32 m_NumDescriptorSets{0};

    // XXX: Need strong reference here?
    // RenderPassRef m_CurrentRenderPass;
    RenderPass* m_CurrentRenderPass{nullptr};
    Framebuffer* m_CurrentFramebuffer{nullptr};
    Pipeline* m_CurrentPipeline;

    // Color targets + 1 depth/stencil value at the end.
    std::array<vk::ClearValue, GPUConstants::MaxRenderTargets + 1> m_ClearValues;

    bool m_IsRecording{false};

    u32 m_CurrentCommand{0};

    // Index to the CommandBuffer array in COmmandBufferManager.
    u32 m_Index{0};

    bool m_IsSecondary{false};

    friend class CommandBufferManager;
};

/*
 * Command buffer with ref counting and automatic resource destruction.
 */
class ImmediateCommandBuffer : public GPUResource
{
public:
    explicit ImmediateCommandBuffer(CommandBufferManager* commandBufferManager);
    virtual ~ImmediateCommandBuffer();
    NON_COPYABLE(ImmediateCommandBuffer);
    NON_MOVEABLE(ImmediateCommandBuffer);

    CommandBuffer* GetCommandBuffer()
    {
        return &m_CommandBuffer;
    }

private:
    CommandBuffer m_CommandBuffer;

    friend class CommandBufferManager;
};

using ImmediateCommandBufferRef = RefCountPtr<ImmediateCommandBuffer>;

class CommandBufferManager
{
public:
    CommandBufferManager() = default;
    ~CommandBufferManager() = default;
    NON_COPYABLE(CommandBufferManager);
    NON_MOVEABLE(CommandBufferManager);

    void Init(VulkanDevice* device, u32 numThreads);
    void Destroy();

    void ResetPools(u32 frameIndex);

    CommandBuffer* GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin);
    CommandBuffer* GetCommandBufferSecondary(u32 frameIndex, u32 threadIndex);

    // Immediate command buffer for eg. texture data upload.
    // ImmediateCommandBufferRef CreateImmediateCommandBuffer();

    inline u32 PoolFromIndex(u32 index) const
    {
        return index / m_NumPoolsPerFrame;
    }
    inline u32 PoolFromIndices(u32 frameIndex, u32 threadIndex) const
    {
        return (frameIndex * m_NumPoolsPerFrame) + threadIndex;
    }

private:
    VulkanDevice* m_Device{nullptr};

    std::vector<CommandBuffer> m_CommandBuffers;
    std::vector<CommandBuffer> m_SecondaryCommandBuffers;

    // Number of used command buffers for a given pool index.
    std::vector<u32> m_NumUsedCommandBuffers;
    std::vector<u32> m_NumUsedSecondaryCommandBuffers;

    // Equal to number of threads.
    u32 m_NumPoolsPerFrame{0};

    u32 m_NumCommandBuffersPerThread{3};
};
