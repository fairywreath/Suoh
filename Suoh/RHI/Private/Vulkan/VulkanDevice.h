#pragma once

#include "VulkanRHI.h"

#include "VulkanCommon.h"
#include "VulkanDeviceManager.h"
#include "VulkanInstance.h"

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
    ~VulkanDevice();

    virtual GraphicsAPI GetGraphicsAPI() const override;

    /*
     * Heap memory.
     */
    virtual HeapHandle CreateHeap(const HeapDesc& desc) override;

    /*
     * Buffers.
     */
    virtual BufferHandle CreateBuffer(const BufferDesc& desc) override;
    virtual void* MapBuffer(IBuffer* buffer, CpuAccessMode access) override;
    virtual void UnmapBuffer(IBuffer* buffer) override;
    virtual MemoryRequirements GetBufferMemoryRequirements(IBuffer* buffer) override;

    /*
     * XXX: Probably would not need this, will use an internal allocator inside the RHI.
     */
    virtual bool BindBufferMemory(IBuffer* buffer, IHeap* heap, u64 offset) override;
    virtual BufferHandle CreateHandleForNativeBuffer(ObjectType objectType, Object buffer, const BufferDesc& desc) override;

    /*
     * Texture/images.
     */
    virtual TextureHandle CreateTexture(const TextureDesc& desc) override;
    virtual MemoryRequirements GetTextureMemoryRequireents(ITexture* texture) override;
    virtual bool BindTextureMemory(ITexture* texture, IHeap* heap, u64 offset) override;
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
    virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;

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
    virtual bool WriteDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& item) override;

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
    virtual void RunGarbageCollection() override;

    virtual Object GetNativeQueue(ObjectType objectType, CommandQueue queue) override;

    virtual bool QueryFeatureSupport(RHIFeature feature, void* pInfo = nullptr, size_t infoSize = 0) override;
    virtual FormatSupport QueryFormatSupport(Format format) override;

private:
    vk::Queue GetGraphicsQueue() const;
    u32 GetGraphicsFamily() const;

    vk::Queue GetComputeQueue() const;
    u32 GetComputeFamily() const;
    bool ComputeCapable() const;

    vk::Queue GetPresentQueue() const;
    u32 GetPresentFamily() const;

private:
    VulkanInstance m_Instance;
    // XXX: Ideally this should be outside of this class and should be used to create instances of this class
    VulkanDeviceManager m_DeviceManager;

    vk::PhysicalDevice m_PhysDevice;
    vk::Device m_Device;
    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;
};

} // namespace Vulkan

} // namespace SuohRHI
