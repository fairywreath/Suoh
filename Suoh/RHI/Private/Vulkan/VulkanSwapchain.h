#pragma once

#include "VulkanCommon.h"

namespace SuohRHI
{

namespace Vulkan
{

class VulkanDevice;

class VulkanSwapchain
{
public:
    VulkanSwapchain() = default;
    VulkanSwapchain(VulkanDevice* device, const VulkanContext& context, u32 width, u32 height);
    ~VulkanSwapchain();

    NON_COPYABLE(VulkanSwapchain);
    NON_MOVEABLE(VulkanSwapchain);

    // XXX: These are kind of sketchy, might want nicer dependency/context constructors.
    void InitSwapchain(VulkanDevice* device, const VulkanContext& context, u32 width, u32 height);
    void Destroy();

    void AcquireNextImage();
    void Present(vk::Semaphore renderSemaphore);

    vk::Image GetImage(std::size_t index) const;
    const vk::ImageView& GetImageView(std::size_t index) const;

    size_t GetImageCount() const;
    size_t GetCurrentImageIndex() const;
    const vk::Semaphore& GetCurrentPresentSemaphore() const;

    vk::Format GetSwapchainImageFormat() const;
    vk::Extent2D GetExtent() const;

private:
    struct SwapchainSupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> surfaceFormats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapchainSupportDetails QuerySwapchainSupport();
    vk::SurfaceFormatKHR ChooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, u32 width, u32 height);

    void InitSemaphores();

private:
    VulkanDevice* m_pDevice{nullptr};
    VulkanContext m_Context;

    vk::SwapchainKHR m_Swapchain;

    vk::Format m_SwapchainImageFormat;
    vk::PresentModeKHR m_PresentMode;
    vk::Extent2D m_Extent;

    std::vector<vk::Image> m_Images;
    std::vector<vk::ImageView> m_ImageViews;

    std::vector<vk::Semaphore> m_PresentSemaphores;

    size_t m_ImageCount{0};
    // Image index returned from AcqureNextImage.
    u32 m_ImageIndex{0};
    // Own internal frame counter to choose present semaphore.
    u32 m_FrameIndex{0};
};

} // namespace Vulkan

} // namespace SuohRHI
