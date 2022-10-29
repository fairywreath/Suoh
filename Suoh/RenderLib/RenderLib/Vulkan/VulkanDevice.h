#pragma once

#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

#include "VulkanBindings.h"
#include "VulkanBuffer.h"
#include "VulkanCommandList.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanShader.h"

namespace RenderLib
{

namespace Vulkan
{

class VulkanDevice
{
public:
    explicit VulkanDevice(const DeviceDesc& desc);
    ~VulkanDevice();

    NON_COPYABLE(VulkanDevice);
    NON_MOVEABLE(VulkanDevice);

    BufferHandle CreateBuffer(const BufferDesc& desc);

    ImageHandle CreateImage(const ImageDesc& desc);
    SamplerHandle CreateSampler(const SamplerDesc& desc);

    ShaderHandle CreateShader(const ShaderDesc& desc);

    RenderPassHandle CreateRenderPass(const RenderPassDesc& desc);
    FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc);
    GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc);

    DescriptorLayoutHandle CreateDescriptorLayout(const DescriptorLayoutDesc& desc);
    DescriptorSetHandle CreateDescriptorSet(const DescriptorSetDesc& desc);

    CommandListHandle CreateCommandList(const CommandListDesc& desc);

    // XXX: Return submission ID?
    void Submit(CommandList* commandList);
    void WaitGraphicsSubmissionSemaphore();
    void WaitTransferSubmissionSemaphore();

    // XXX: expose synchronization primitives?
    void Present();

    void WaitIdle();

    void* MapBuffer(Buffer* buffer);
    void UnmapBuffer(Buffer* buffer);

    // Swapchain image access.
    const std::vector<ImageHandle>& GetSwapchainImages() const
    {
        return m_SwapchainImages;
    }
    const u32 GetCurrentSwapchainImageIndex() const
    {
        return m_Swapchain.GetCurrentImageIndex();
    }
    const u32 GetSwapchainImageCount() const
    {
        return m_Swapchain.GetImageCount();
    }
    void SwapchainAcquireNextImage();

    // Misc.
    vk::Format FindSupportedFormat(const std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features);
    vk::Format FindDepthFormat();

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
    void InitAllocator();

    void InitSwapchainImages();

    void InitSynchronizationObjects();

private:
    // Create instance when creating device, destroy instance when instance is destroyed.
    // Ideally instance should be outisde this device and should be used to create this device
    // itself.
    VulkanInstance m_Instance;

    VulkanSwapchain m_Swapchain;

    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;
    vk::Queue m_ComputeQueue;

    u32 m_GraphicsFamily;
    u32 m_PresentFamily;
    u32 m_ComputeFamily;

    // Context to inject to other classes/types when creating resources.
    VulkanContext m_Context;

    // Native swapchain resources transformed into new wrappers.
    std::vector<ImageHandle> m_SwapchainImages;

    // Render semaphore, wait for this on present, signal on graphics submission.
    vk::Semaphore m_RenderSemaphore;

    // Queue submission synchronization.
    // XXX: Have a dedicated VulkanQueue class to handle this?
    vk::Semaphore m_GraphicsSubmissionSemaphore;
    u64 m_LastSubmittedGraphicsID{0};
    u64 m_LastFinishedGraphicsID{0};

    // Not used, since we do not have a transfer queue...
    vk::Semaphore m_TransferSubmissionSemaphore;
    u64 m_LastSubmittedTransferID{0};
    u64 m_LastFinishedTransferID{0};
};

} // namespace Vulkan

} // namespace RenderLib
