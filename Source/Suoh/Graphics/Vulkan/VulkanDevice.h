#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include "GPUProfiler.h"

#include "VulkanBarriers.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDescriptorSets.h"
#include "VulkanShaderState.h"
#include "VulkanSwapchain.h"

struct VulkanDeviceDesc
{
    void* window{nullptr};
    u32 width{1};
    u32 height{1};

    u32 gpuTimeQueriesPerFrame{32};
    u32 numThreads{1};

    bool enableTimeQueries{false};
    bool enablePipelineStatistics{true};

    bool useDynamicRendering{true};

    VulkanDeviceDesc& SetWindow(u32 _width, u32 _height, void* _window)
    {
        width = _width;
        height = _height;
        window = _window;
        return *this;
    }
    VulkanDeviceDesc& SetNumThreads(u32 value)
    {
        numThreads = value;
        return *this;
    }
    VulkanDeviceDesc& SetTimeQueries(bool value)
    {
        enableTimeQueries = value;
        return *this;
    }
    VulkanDeviceDesc& SetPipelineStatistics(bool value)
    {
        enablePipelineStatistics = value;
        return *this;
    }
    VulkanDeviceDesc& SetDynamicRendering(bool value)
    {
        useDynamicRendering = value;
        return *this;
    }
};

class DeferredDeletionQueue
{
public:
    explicit DeferredDeletionQueue(VulkanDevice* device);
    ~DeferredDeletionQueue();

    enum class Type
    {
        BufferAllocated,
        TextureAllocated,
        TextureView,
        Sampler,
        ShaderModule,
        DescriptorSetLayout,
        DescriptorSet,
        RenderPass,
        Framebuffer,
        Pipeline,
        PipelineLayout,
    };

    template <typename T>
    inline void EnqueueResource(Type type, T handle)
    {
        EnqueueVulkanHandle(type, ToVulkanHandleU64(handle));
    }

    template <typename T>
    inline void EnqueueAllocatedResource(Type type, T handle, VmaAllocation allocation)
    {
        EnqueueVulkanAllocation(type, ToVulkanHandleU64(handle), allocation);
    }

    void DestroyResources();

private:
    void EnqueueVulkanHandle(Type type, u64 handle);
    void EnqueueVulkanAllocation(Type type, u64 handle, VmaAllocation allocation);

private:
    struct Entry
    {
        Type type;
        u64 vulkanHandle;
        VmaAllocation allocation;

        // XXX TODO: misc. info on when it is safe to delete.
        // u32 frameNumber;
    };

    std::vector<Entry> m_Entries;

    VulkanDevice* m_Device;
};

class VulkanDevice
{
public:
    explicit VulkanDevice(const VulkanDeviceDesc& desc);
    ~VulkanDevice();

    NON_COPYABLE(VulkanDevice);
    NON_MOVEABLE(VulkanDevice);

    /*
     * Resource creation.
     */
    BufferRef CreateBuffer(const BufferDesc& desc);

    TextureRef CreateTexture(const TextureDesc& desc);
    TextureRef CreateTextureView(const TextureViewDesc& desc);

    SamplerRef CreateSampler(const SamplerDesc& desc);

    DescriptorSetLayoutRef CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc);
    DescriptorSetRef CreateDescriptorSet(const DescriptorSetDesc& desc);

    ShaderStateRef CreateShaderState(const ShaderStateDesc& desc);
    RenderPassRef CreateRenderPass(const RenderPassDesc& desc);
    FramebufferRef CreateFramebuffer(const FramebufferDesc& desc);

    PipelineRef CreatePipeline(const PipelineDesc& desc);

    /*
     * Resource CPU mapping.
     */
    void* MapBuffer(const MapBufferParameters& params);
    void UnmapBuffer(const MapBufferParameters& params);
    void* DynamicAllocate(u32 size);

    /*
     * Resource update.
     */
    void ResizeOutputTextures(FramebufferRef renderPass, u32 width, u32 height);
    void ResizeTexture(TextureRef texture, u32 width, u32 height);

    void QueueUpdateDescriptorSet(DescriptorSet* set);
    void UpdateDescriptorSet(DescriptorSet* set);

    /*
     * Command buffers.
     */
    CommandBuffer* GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin);
    CommandBuffer* GetCommandBufferSecondary(u32 frameIndex, u32 threadIndex);

    void QueueCommandBuffer(CommandBuffer* commandBuffer);
    void SubmitQueuedCommandBuffers();
    void SubmitComputeCommandBuffer(CommandBuffer* commandBuffer);
    // void SubmitCommandBuffer(CommandBuffer* commandBuffer);
    // void SubmitImmediateCommandBuffer(ImmediateCommandBuffer* commandBuffer);

    /*
     * Rendering.
     */
    void NewFrame();
    void Present(CommandBuffer* asyncComputeCommandBuffer = nullptr);
    void Resize();

    void AddBindlessResourceUpdate(const ResourceUpdate& update);

    ExecutionBarrier FillBarrier(Framebuffer* frameBuffer);

    // bool BufferReady(BufferRef buffer);

    BufferRef GetFullScreenVertexBuffer() const
    {
        return m_FullScreenVertexBuffer;
    }
    RenderPassRef GetSwapchainRenderPass() const;
    FramebufferRef GetCurrentFramebuffer() const;
    TextureRef GetDummyTexture() const
    {
        return m_DummyTexture;
    }
    BufferRef GetDummyConstantBuffer() const
    {
        return m_DummyConstantBuffer;
    }

    const RenderPassOutput& GetSwapchainRenderPassOutput() const
    {
        return m_Swapchain.GetRenderPassOutput();
    }

    /*
     * Debug.
     */
    template <typename T>
    void SetVulkanResourceName(vk::ObjectType type, T handle, const std::string& name)
    {
        SetVulkanHandleResourceName(type, ToVulkanHandleU64(handle), name);
    }

    // void PushMarker(vk::CommandBuffer commandBuffer, const std::string& name);
    // void PopMarker(vk::CommandBuffer commandBuffer);

    /*
     * Misc.
     */
    void LinkTextureSampler(Texture* texture, Sampler* sampler) const;
    void AdvanceFrameCounters();

    /*
     * GPU timestamps.
     */
    void SetGPUTimestampsEnable(bool value)
    {
        m_TimestampsEnabled = true;
    }
    // u32 CopyGPUTimeStamps(GPUTimeQuery* outTimestamps);

    /*
     * Device misc info.
     */
    std::string_view GetGPUName() const;

    vk::Instance GetVulkanInstance() const
    {
        return m_VulkanInstance;
    }

    vk::Device GetVulkanDevice() const
    {
        return m_VulkanDevice;
    }

    vk::PhysicalDevice GetVulkanPhysicalDevice() const
    {
        return m_VulkanPhysicalDevice;
    }

    const vk::AllocationCallbacks* GetVulkanAllocationCallbacks() const
    {
        return m_VulkanAllocationCallbacks;
    }

    VmaAllocator GetVmaAllocator() const
    {
        return m_VmaAllocator;
    }

    vk::Queue GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

    u32 GetGraphicsQueueFamily() const
    {
        return m_GraphicsQueueFamily;
    }

    std::vector<GPUThreadFramePools>& GetThreadFramePools()
    {
        return m_ThreadFramePools;
    }

    u32 GetCurrentFrameIndex() const
    {
        return m_CurrentFrame;
    }

    bool UseDynamicRendering() const
    {
        return (!m_DeviceDesc.useDynamicRendering && m_SupportedExtensions.dynamicRendering);
    }

    bool UseBindless() const
    {
        return m_BindlessSupported;
    }

    vk::DescriptorSet GetVulkanBindlessDescriptorSet() const
    {
        return m_VulkanDescriptorSetBindlessCached;
    }

    BufferRef GetDynamicBuffer() const
    {
        return m_DynamicBuffer;
    }

    const vk::PhysicalDeviceProperties& GetVulkanPhysicalDeviceProperties() const
    {
        return m_VulkanPhysicalDeviceProperties;
    }

    DeferredDeletionQueue& GetDeferredDeletionQueue()
    {
        return m_DeferredDeletionQueue;
    }

    vk::DescriptorPool GetVulkanGlobalDescriptorPool() const
    {
        return m_VulkanGlobalDescriptorPool;
    }

    vk::DescriptorPool GetVulkanBindlessDescriptorPool() const
    {
        return m_VulkanGlobalDescriptorPoolBindless;
    }

    // XXX: This is not tied to this class, put somwhere else?
    RenderPassOutput FillRenderPassOutput(const RenderPassDesc& desc) const;

private:
    void Init(const VulkanDeviceDesc& desc);
    void Destroy();

    void InitVulkanInstance();
    void InitVulkanDebugUtils();
    void InitVulkanPhysicalDevice(const VulkanDeviceDesc& deviceDesc);
    void InitVulkanLogicalDevice();

    void InitVulkanAllocator();
    void InitVulkanGlobalDescriptorPools();
    void InitVulkanSynchronizationStructures();

    // Creates per thread frame command + query + pipeline stats query pools.
    void InitThreadFramePools(u32 numThreads, u32 timeQueriesPerFrame);

    void SetVulkanHandleResourceName(vk::ObjectType objectType, u64 handle, const std::string& name);

    void UploadTextureData(Texture* texture, void* textureData);

    // Calculate graphics timeline semaphore wait value.
    u64 GetGraphicsSemaphoreWaitValue() const
    {
        // XXX: This can overflow to negative.
        return m_AbsoluteFrame - (GPUConstants::MaxFrames - 1);
    }
    void GetQueryPoolResults();
    void UpdateBindlessTextures();

private:
    VulkanDeviceDesc m_DeviceDesc;

    vk::DynamicLoader m_VulkanDynamicLoader;
    vk::AllocationCallbacks* m_VulkanAllocationCallbacks{nullptr};

    vk::Instance m_VulkanInstance;

    vk::PhysicalDevice m_VulkanPhysicalDevice;
    vk::PhysicalDeviceProperties m_VulkanPhysicalDeviceProperties;

    vk::Device m_VulkanDevice;

    vk::Queue m_GraphicsQueue;
    vk::Queue m_ComputeQueue;
    vk::Queue m_TransferQueue;
    vk::Queue m_PresentQueue;

    u32 m_GraphicsQueueFamily;
    u32 m_ComputeQueueFamily;
    u32 m_TransferQueueFamily;

    Swapchain m_Swapchain;

    VmaAllocator m_VmaAllocator;

    vk::DescriptorPool m_VulkanGlobalDescriptorPool;
    vk::DescriptorPool m_VulkanGlobalDescriptorPoolBindless;
    vk::DescriptorSet m_VulkanDescriptorSetBindlessCached;

    vk::DebugUtilsMessengerEXT m_VulkanDebugUtilsMessenger;

    u32 m_NumThreads;

    u32 m_TimestampFrequency;

    u32 m_CurrentFrame{0};
    u32 m_PreviousFrame{0};
    u64 m_AbsoluteFrame{0};

    /*
     * Per frame synchronization.
     */

    // Present semaphores.
    std::array<vk::Semaphore, GPUConstants::MaxFrames> m_VulkanSemaphoresRenderComplete;

    // Swapchain image acquire semaphore.
    vk::Semaphore m_VulkanSemaphoreImageAcquired;

    // Graphics work timeline semaphore.
    vk::Semaphore m_VulkanSemaphoreGraphics;

    // Async compute timline semaphore.
    vk::Semaphore m_VulkanSemaphoreCompute;

    u64 m_LastComputeSemaphoreValue{0};
    bool m_HasAsyncWork{false};

    /*
     * Internal GPU resources.
     */
    BufferRef m_FullScreenVertexBuffer;
    RenderPassRef m_SwapchainRenderPass;
    SamplerRef m_DefaultSampler;
    BufferRef m_DummyConstantBuffer;
    TextureRef m_DummyTexture;

    /*
     * Dynamic buffer.
     */
    BufferRef m_DynamicBuffer;
    u32 m_DynamicMaxPerFrameSize{};
    u8* m_DynamicMappedMemory{nullptr};
    u32 m_DynamicAllocatedSize{0};
    u32 m_DynamicPerFrameSize;

    /*
     * Command buffers.
     */
    CommandBufferManager m_CommandBufferManager;
    std::vector<CommandBuffer*> m_QueuedCommandBuffers;
    // u32 m_NumAllocatedCommandBuffers{0};
    // u32 m_NumQueuedCommandBuffers{0};

    std::vector<GPUThreadFramePools> m_ThreadFramePools;

    // std::vector<ResourceUpdate> m_ResourceDeletionQueue;
    DeferredDeletionQueue m_DeferredDeletionQueue;

    std::vector<ResourceUpdate> m_BindlessTextureUpdates;
    std::vector<DescriptorSetUpdate> m_DescriptorSetUpdates;

    size_t m_UBOAlignment{256};
    size_t m_SSBOAlignment{256};

    PresentMode m_PresentMode{PresentMode::VSync};

    bool m_BindlessSupported{false};
    bool m_TimestampsEnabled{false};
    bool m_Resized{false};
    bool m_Vsync{false};

    struct SupportedExtensions
    {
        bool debugUtils{false};

        bool timelineSemaphore{false};
        bool synchronization2{false};
        bool dynamicRendering{false};
        bool meshShaders{false};
        bool rayTracing{false};
    } m_SupportedExtensions;

    // XXX: Remove these and do better encapsulation?
    // friend class Buffer;
    // friend class Sampler;
    // friend class Texture;
    // friend class DescriptorSetLayout;
    // friend class DescriptorSet;
    // friend class ShaderState;
    // friend class RenderPass;
    // friend class Framebuffer;
    // friend class Pipeline;

    // friend class Swapchain;
    // friend class CommandBuffer;
    // friend class CommandBufferManager;
};