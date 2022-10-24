#pragma once

#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

#include "VulkanBuffer.h"
#include "VulkanCommandList.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanShader.h"

namespace RenderCore
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

    CommandListHandle CreateCommandList(const CommandListDesc& desc);

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

    void InitSwapchainImage();

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

    // Internal command pool and buffer for upload/transition tasks.

    // Native swapchain resources transformed into new wrappers.
    std::vector<ImageHandle> m_SwapchainImages;
};

} // namespace Vulkan

} // namespace RenderCore
