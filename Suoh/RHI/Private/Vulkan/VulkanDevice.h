#pragma once

#include "VulkanRHI.h"

#include "VulkanCommon.h"
#include "VulkanDeviceManager.h"
#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

#include "VulkanBindings.h"
#include "VulkanBuffer.h"
#include "VulkanCommandList.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanQueue.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanUploadManager.h"
#include "VulkanUtils.h"

namespace SuohRHI
{

namespace Vulkan
{

/*
 * Vulkan Logical Device.
 */
class VulkanDevice : public RefCountResource<IDevice>
{
public:
    explicit VulkanDevice(const DeviceDesc& desc);
    ~VulkanDevice() override;

    virtual GraphicsAPI GetGraphicsAPI() const override;
    Object GetNativeObject(ObjectType type) override;

    /*
     * Buffers.
     */
    virtual BufferHandle CreateBuffer(const BufferDesc& desc) override;
    virtual void* MapBuffer(IBuffer* buffer, CpuAccessMode access) override;
    virtual void UnmapBuffer(IBuffer* buffer) override;
    virtual BufferHandle CreateHandleForNativeBuffer(ObjectType objectType, Object buffer, const BufferDesc& desc) override;

    /*
     * Texture/images.
     */
    virtual TextureHandle CreateTexture(const TextureDesc& desc) override;
    virtual TextureHandle CreateHandleForNativeTexture(ObjectType objectType, Object texture, const TextureDesc& desc) override;

    virtual StagingTextureHandle CreateStagingTexture(const TextureDesc& desc, CpuAccessMode access) override;
    virtual void* MapStagingTexture(IStagingTexture* texture, const TextureLayer& textureLayer, CpuAccessMode access,
                                    size_t* outRowPitch) override;
    virtual void UnmapStagingTexture(IStagingTexture* texture) override;

    /*
     * Shaders.
     */
    virtual ShaderHandle CreateShader(const ShaderDesc& desc, const void* binary, size_t binarySize) override;
    virtual ShaderLibraryHandle CreateShaderLibrary(const void* binary, size_t binarySize) override;

    /*
     * Samplers.
     */
    virtual SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    virtual InputLayoutHandle CreateInputLayout(const VertexAttributeDesc* desc, u32 attributeCount, IShader* vertexShader) override;

    /*
     * Event/timer Queries.
     */
    virtual EventQueryHandle CreateEventQuery() override;
    virtual void SetEventQuery(IEventQuery* query, CommandQueue queue) override;
    virtual bool PollEventQuery(IEventQuery* query) override;
    virtual void WaitEventQuery(IEventQuery* query) override;
    virtual void ResetEventQuery(IEventQuery* query) override;

    virtual TimerQueryHandle CreateTimerQuery() override;
    virtual bool PollTimerQuery(ITimerQuery* query) override;
    virtual float GetTimerQueryTimeSeconds(ITimerQuery* query) override;
    virtual void ResetTimerQuerySeconds(ITimerQuery* query) override;

    /*
     * Graphics Pipeline.
     */
    virtual FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) override;
    virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer) override;

    /*
     * Compute.
     */
    virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) override;

    /*
     * Descriptor sets/layouts.
     */
    virtual BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc) override;
    virtual BindingLayoutHandle CreateBindlessLayout(const BindlessLayoutDesc& desc) override;
    virtual BindingSetHandle CreateBindingSet(const BindingSetDesc& desc, IBindingLayout* layout) override;
    virtual DescriptorTableHandle CreateDescriptorTable(IBindingLayout* layout) override;
    virtual void ResizeDescriptorTable(IDescriptorTable* descriptorTable, u32 newSize, bool keepContents = true) override;
    virtual bool WriteDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& binding) override;

    /*
     * Command List.
     * XXX: Maybe have an internal "Job" handled by the RHI internally?
     * Something like:
     *      Receipt SubmitJob(Context context)
     * Having different contexts for each job type, eg. RenderContet, ComputContext, RayTracingContext would be great
     */
    virtual CommandListHandle CreateCommandList(const CommandListParameters& params = CommandListParameters()) override;
    virtual u64 ExecuteCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists,
                                    CommandQueue executionQueue = CommandQueue::GRAPHICS) override;

    virtual void QueueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, u64 instance) override;
    virtual void WaitForIdle() override;

    /*
     * Present.
     */
    virtual void Present(ITexture* texture) override;

    /*
     * Misc.
     */
    virtual void Cleanup() override;

    virtual Object GetNativeQueue(ObjectType objectType, CommandQueue queue) override;

    virtual bool QueryFeatureSupport(RHIFeature feature, void* pInfo = nullptr, size_t infoSize = 0) override;
    virtual FormatSupport QueryFormatSupport(Format format) override;

    vk::Semaphore GetQueueSemaphore(CommandQueue queue);
    void QueueWaitForSemaphore(CommandQueue waitQueue, vk::Semaphore semaphore, u64 value);
    void QueueSignalSemaphore(CommandQueue executionQueue, vk::Semaphore semaphore, u64 value);
    u64 QueueGetCompletedInstance(CommandQueue queue);

    FramebufferHandle CreateHandleForNativeFramebuffer(vk::RenderPass renderPass, vk::Framebuffer framebuffer, const FramebufferDesc& desc,
                                                       bool transferOwnership);

    /*
     * VulkanDevice specific.
     */
    VulkanQueue* GetQueue(CommandQueue queue) const
    {
        return m_Queues[size_t(queue)].get();
    }

    u32 GetGraphicsFamily() const
    {
        return m_GraphicsFamily;
    }
    u32 GetPresentFamily() const
    {
        return m_PresentFamily;
    }
    u32 GetComputeFamily() const
    {
        return m_ComputeFamily;
    }
    vk::Queue GetPresentQueue() const
    {
        return m_PresentQueue;
    }
    vk::Queue GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

private:
    void* MapBuffer(IBuffer* b, CpuAccessMode flags, uint64_t offset, size_t size) const;

    void InitAllocator();

private:
    VulkanInstance m_Instance;
    // XXX: Ideally this should be outside of this class and should be used to create instances of this class
    VulkanDeviceManager m_DeviceManager;

    VulkanSwapchain m_Swapchain;

    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;
    vk::Queue m_ComputeQueue;

    u32 m_GraphicsFamily;
    u32 m_PresentFamily;
    u32 m_ComputeFamily;

    // Context to inject to other classes/types when creating resources
    VulkanContext m_Context;

    std::array<std::unique_ptr<VulkanQueue>, u32(CommandQueue::COUNT)> m_Queues;

    std::mutex m_Mutex;
};

} // namespace Vulkan

} // namespace SuohRHI
