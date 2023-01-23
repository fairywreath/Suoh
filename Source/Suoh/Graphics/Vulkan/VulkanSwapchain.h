#pragma once

#include "VulkanResources.h"

struct SwapchainDesc
{
    // Native SDL window handle.
    void* window{nullptr};

    SwapchainDesc& SetWindow(void* _window)
    {
        window = _window;
        return *this;
    }
};

class Swapchain
{
public:
    Swapchain() = default;
    ~Swapchain() = default;

    NON_COPYABLE(Swapchain);
    NON_MOVEABLE(Swapchain);

    void Init(VulkanDevice* device, const SwapchainDesc& desc);
    void Destroy();

    void Resize();

    vk::Result AcquireNextImage(vk::Semaphore imageAcquiredSemaphore);
    vk::Result Present(vk::Semaphore renderCompleteSemaphore);

    // Get index of acquired image.
    u32 GetCurrentVulkanImageIndex() const
    {
        return m_VulkanImageIndex;
    }

    vk::SurfaceFormatKHR GetVulkanSurfaceFormat() const
    {
        return m_VulkanSurfaceFormat;
    }

    vk::PresentModeKHR GetVulkanPresentMode() const
    {
        return m_VulkanPresentMode;
    }

    u32 GetSwapchainImageCount() const
    {
        return m_SwapchainImageCount;
    }

    u32 GetSwapchainWidth() const
    {
        return m_SwapchainWidth;
    }

    u32 GetSwapchainHeight() const
    {
        return m_SwapchainHeight;
    }

    const RenderPassOutput& GetRenderPassOutput() const
    {
        return m_RenderPassOutput;
    }

private:
    void InitVulkanSurface();
    void InitVulkanSwapchain();

    vk::SurfaceFormatKHR ChooseSurfaceFormat();
    vk::PresentModeKHR ChooseSurfacePresentMode(vk::PresentModeKHR presentMode);

private:
    VulkanDevice* m_Device;
    void* m_NativeWindowPtr;

    vk::SurfaceKHR m_VulkanSurface;
    vk::SurfaceFormatKHR m_VulkanSurfaceFormat;
    vk::PresentModeKHR m_VulkanPresentMode;
    vk::SwapchainKHR m_VulkanSwapchain;

    // XXX: Probably do not need these since we only need to create the swapchain framebuffers.
    std::vector<vk::Image> m_SwapchainImages;
    std::vector<vk::ImageView> m_SwapchainImageViews;

    // std::array<FramebufferRef, GPUConstants::MaxSwapchainImages> m_Framebuffers;
    std::vector<FramebufferRef> m_Framebuffers;

    u32 m_SwapchainImageCount;
    vk::Extent2D m_SwapchainExtent;
    u32 m_SwapchainWidth;
    u32 m_SwapchainHeight;

    // Vulkan image acquired index.
    u32 m_VulkanImageIndex;

    RenderPassOutput m_RenderPassOutput;
};