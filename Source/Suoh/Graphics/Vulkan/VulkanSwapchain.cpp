#include "VulkanSwapchain.h"
#include "GPUConstants.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <Core/Logger.h>

void Swapchain::Init(VulkanDevice* device, const SwapchainDesc& desc)
{
    m_Device = device;
    m_NativeWindowPtr = desc.window;

    InitVulkanSurface();
    InitVulkanSwapchain();
}

void Swapchain::Destroy()
{
    // XXX: Destroy framebuffers/image views.

    m_Device->GetVulkanDevice().destroySwapchainKHR(m_VulkanSwapchain);
    m_Device->GetVulkanInstance().destroySurfaceKHR(m_VulkanSurface);
}

void Swapchain::InitVulkanSurface()
{
    auto vulkanInstance = m_Device->GetVulkanInstance();
    SDL_Window* window = (SDL_Window*)m_NativeWindowPtr;

    if (SDL_Vulkan_CreateSurface(window, vulkanInstance, reinterpret_cast<VkSurfaceKHR*>(&m_VulkanSurface))
        == SDL_FALSE)
    {
        LOG_FATAL("Failed to create vulkan surface!");
    }
}

void Swapchain::InitVulkanSwapchain()
{
    const auto vulkanDevice = m_Device->GetVulkanDevice();
    const auto vulkanPhysDevice = m_Device->GetVulkanPhysicalDevice();

    m_RenderPassOutput.Reset();
    m_VulkanSurfaceFormat = ChooseSurfaceFormat();
    m_RenderPassOutput.SetDepthStencilTarget(vk::Format::eD32Sfloat, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    m_RenderPassOutput.SetDepthStencilOperations(RenderPassOperation::Clear, RenderPassOperation::Clear);

    m_VulkanPresentMode = ChooseSurfacePresentMode(vk::PresentModeKHR::eFifo);
    auto capabilities = vulkanPhysDevice.getSurfaceCapabilitiesKHR(m_VulkanSurface);

    m_SwapchainExtent = capabilities.currentExtent;
    if (m_SwapchainExtent.width == u32(-1))
    {
        m_SwapchainExtent.width
            = std::clamp(m_SwapchainExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        m_SwapchainExtent.height = std::clamp(m_SwapchainExtent.height, capabilities.minImageExtent.height,
                                              capabilities.maxImageExtent.height);
    }

    u32 imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR()
                          .setSurface(m_VulkanSurface)
                          .setMinImageCount(imageCount)
                          .setImageFormat(m_VulkanSurfaceFormat.format)
                          .setImageColorSpace(m_VulkanSurfaceFormat.colorSpace)
                          .setImageExtent(m_SwapchainExtent)
                          .setImageArrayLayers(1)
                          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                          .setPreTransform(capabilities.currentTransform)
                          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                          .setPresentMode(m_VulkanPresentMode)
                          .setClipped(true)
                          .setOldSwapchain(nullptr);

    // XXX: Use dedicated present queue family?
    u32 graphicsFamily = m_Device->GetGraphicsQueueFamily();
    u32 presentFamily = graphicsFamily;
    const std::array queueFamilyIndices = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily)
    {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent).setQueueFamilyIndices(queueFamilyIndices);
    }
    else
    {
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    }

    vk::Result vkRes;

    vkRes = vulkanDevice.createSwapchainKHR(&createInfo, nullptr, &m_VulkanSwapchain);
    VK_CHECK(vkRes);

    m_SwapchainWidth = m_SwapchainExtent.width;
    m_SwapchainHeight = m_SwapchainExtent.height;

    m_SwapchainImages = vulkanDevice.getSwapchainImagesKHR(m_VulkanSwapchain);
    m_SwapchainImageCount = m_SwapchainImages.size();

    // Create image views.
    // m_SwapchainImageViews.resize(m_SwapchainImageCount);
    // for (size_t i = 0; i < m_SwapchainImageCount; i++)
    // {
    //     auto imageViewCreateInfo
    //         = vk::ImageViewCreateInfo()
    //               .setImage(m_SwapchainImages[i])
    //               .setViewType(vk::ImageViewType::e2D)
    //               .setFormat(m_VulkanSurfaceFormat.format)
    //               .setComponents(vk::ComponentMapping()
    //                                  .setR(vk::ComponentSwizzle::eIdentity)
    //                                  .setG(vk::ComponentSwizzle::eIdentity)
    //                                  .setB(vk::ComponentSwizzle::eIdentity)
    //                                  .setA(vk::ComponentSwizzle::eIdentity))
    //               .setSubresourceRange(vk::ImageSubresourceRange()
    //                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
    //                                        .setBaseMipLevel(0)
    //                                        .setLevelCount(1)
    //                                        .setBaseArrayLayer(0)
    //                                        .setLayerCount(1));

    //     vkRes = m_Device->m_VulkanDevice.createImageView(&imageViewCreateInfo, nullptr,
    //                                                      &m_SwapchainImageViews[i]);
    //     VK_CHECK(vkRes);
    // };

    // XXX: Create swapchain framebuffers.
    // XXX: Transition swapchain images.
}

void Swapchain::Resize()
{
    auto vulkanDevice = m_Device->GetVulkanDevice();
    auto vulkanPhysDevice = m_Device->GetVulkanPhysicalDevice();

    vulkanDevice.waitIdle();

    auto capabilities = vulkanPhysDevice.getSurfaceCapabilitiesKHR(m_VulkanSurface);
    auto swapchainExtent = capabilities.currentExtent;

    if (swapchainExtent.width == 0 || swapchainExtent.height == 0)
    {
        LOG_ERROR("Resize on zero length swapchain extent requested!");
        return;
    }

    Destroy();

    InitVulkanSurface();
    InitVulkanSwapchain();

    vulkanDevice.waitIdle();
}

vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat()
{
    auto surfaceFormats = m_Device->GetVulkanPhysicalDevice().getSurfaceFormatsKHR(m_VulkanSurface);
    for (const auto& format : surfaceFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            m_RenderPassOutput.AddColorTarget(format.format, vk::ImageLayout::ePresentSrcKHR,
                                              RenderPassOperation::Clear);
            return format;
        }
    }

    LOG_ERROR("Unable to find suitable surface format!");
    return surfaceFormats[0];
}

vk::PresentModeKHR Swapchain::ChooseSurfacePresentMode(vk::PresentModeKHR requestedPresentMode)
{
    const auto presentModes = m_Device->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_VulkanSurface);
    for (auto mode : presentModes)
    {
        if (mode == requestedPresentMode)
            return mode;
    }

    LOG_ERROR("VulkanSwapchain: Failed to get requested surface present mode!");
    return vk::PresentModeKHR::eFifo;
}

vk::Result Swapchain::AcquireNextImage(vk::Semaphore imageAcquiredSemaphore)
{
    auto vulkanDevice = m_Device->GetVulkanDevice();
    const auto vkRes = vulkanDevice.acquireNextImageKHR(m_VulkanSwapchain, u64(-1), imageAcquiredSemaphore, nullptr,
                                                        &m_VulkanImageIndex);
    return vkRes;
}

vk::Result Swapchain::Present(vk::Semaphore renderCompleteSemaphore)
{
    auto presentInfo = vk::PresentInfoKHR()
                           .setWaitSemaphores(renderCompleteSemaphore)
                           .setSwapchains(m_VulkanSwapchain)
                           .setImageIndices(m_VulkanImageIndex);
    const auto vkRes = m_Device->GetGraphicsQueue().presentKHR(presentInfo);
    return vkRes;
}