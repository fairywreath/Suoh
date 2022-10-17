#include "VulkanSwapchain.h"

#include "VulkanDevice.h"

namespace SuohRHI
{

namespace Vulkan
{

static constexpr auto ACQUIRE_TIMEOUT = 1000000000;

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, const VulkanContext& context, u32 width, u32 height)
    : m_pDevice(device), m_Context(context)
{
    InitSwapchain(device, context, width, height);
}

VulkanSwapchain::~VulkanSwapchain()
{
    if (m_pDevice != nullptr)
        Destroy();
}

void VulkanSwapchain::Destroy()
{
    for (auto semaphore : m_PresentSemaphores)
    {
        m_Context.device.destroySemaphore(semaphore);
    }
    for (auto view : m_ImageViews)
    {
        m_Context.device.destroyImageView(view);
    }

    m_Context.device.destroySwapchainKHR(m_Swapchain);

    m_pDevice = nullptr;
}

void VulkanSwapchain::AcquireNextImage()
{
    const vk::Result res
        = m_Context.device.acquireNextImageKHR(m_Swapchain, ACQUIRE_TIMEOUT, m_PresentSemaphores[m_FrameIndex], vk::Fence(), &m_ImageIndex);
    assert(res == vk::Result::eSuccess);
}

void VulkanSwapchain::Present(vk::Semaphore renderSemaphore)
{
    // XXX: use own VulkanQueue class.
    auto presentInfo = vk::PresentInfoKHR()
                           .setWaitSemaphoreCount(1)
                           .setPWaitSemaphores(&renderSemaphore)
                           .setSwapchainCount(1)
                           .setPSwapchains(&m_Swapchain)
                           .setPImageIndices(&m_ImageIndex);

    auto res = m_pDevice->GetPresentQueue().presentKHR(presentInfo);
    VK_ASSERT_OK(res);

    m_FrameIndex = (m_FrameIndex + 1) % m_ImageCount;
}

vk::Image VulkanSwapchain::GetImage(std::size_t index) const
{
    return m_Images[index];
}

const vk::ImageView& VulkanSwapchain::GetImageView(std::size_t index) const
{
    return m_ImageViews[index];
}

size_t VulkanSwapchain::GetImageCount() const
{
    return m_ImageCount;
}

size_t VulkanSwapchain::GetCurrentImageIndex() const
{
    return m_ImageIndex;
}

const vk::Semaphore& VulkanSwapchain::GetCurrentPresentSemaphore() const
{
    return m_PresentSemaphores[m_FrameIndex];
}

vk::Format VulkanSwapchain::GetSwapchainImageFormat() const
{
    return m_SwapchainImageFormat;
}

vk::Extent2D VulkanSwapchain::GetExtent() const
{
    return m_Extent;
}

void VulkanSwapchain::InitSwapchain(VulkanDevice* pDevice, const VulkanContext& context, u32 width, u32 height)
{
    m_pDevice = std::move(pDevice);
    m_Context = context;

    const auto device = m_Context.device;

    auto swapchainSupport = QuerySwapchainSupport();
    auto surfaceFormat = ChooseSwapchainSurfaceFormat(swapchainSupport.surfaceFormats);
    m_SwapchainImageFormat = surfaceFormat.format;
    m_PresentMode = ChooseSwapchainPresentMode(swapchainSupport.presentModes);
    m_Extent = ChooseSwapchainExtent(swapchainSupport.capabilities, width, height);

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount)
    {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR()
                          .setSurface(m_Context.surface)
                          .setMinImageCount(imageCount)
                          .setImageFormat(surfaceFormat.format)
                          .setImageColorSpace(surfaceFormat.colorSpace)
                          .setImageExtent(m_Extent)
                          .setImageArrayLayers(1)
                          .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                          .setPreTransform(swapchainSupport.capabilities.currentTransform)
                          .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                          .setPresentMode(m_PresentMode)
                          .setClipped(true)
                          .setOldSwapchain(nullptr);

    u32 graphicsFamily = m_pDevice->GetGraphicsFamily();
    u32 presentFamily = m_pDevice->GetPresentFamily();
    const std::array<u32, 2> queueFamilyIndices = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily)
    {
        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndexCount(queueFamilyIndices.size())
            .setPQueueFamilyIndices(queueFamilyIndices.data());
    }
    else
    {
        createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    }

    auto res = device.createSwapchainKHR(&createInfo, nullptr, &m_Swapchain);
    VK_ASSERT_OK(res);

    // Setup images.
    m_Images = device.getSwapchainImagesKHR(m_Swapchain);
    m_ImageCount = m_Images.size();

    // Setup image views.
    m_ImageViews.resize(m_ImageCount);
    for (size_t i = 0; i < m_ImageCount; i++)
    {
        auto imageViewCreateInfo = vk::ImageViewCreateInfo()
                                       .setImage(m_Images[i])
                                       .setViewType(vk::ImageViewType::e2D)
                                       .setFormat(m_SwapchainImageFormat)
                                       .setComponents(vk::ComponentMapping()
                                                          .setR(vk::ComponentSwizzle::eIdentity)
                                                          .setG(vk::ComponentSwizzle::eIdentity)
                                                          .setB(vk::ComponentSwizzle::eIdentity)
                                                          .setA(vk::ComponentSwizzle::eIdentity))
                                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                .setBaseMipLevel(0)
                                                                .setLevelCount(1)
                                                                .setBaseArrayLayer(0)
                                                                .setLayerCount(1));

        res = device.createImageView(&imageViewCreateInfo, nullptr, &m_ImageViews[i]);
        VK_ASSERT_OK(res);
    };

    InitSemaphores();
}

VulkanSwapchain::SwapchainSupportDetails VulkanSwapchain::QuerySwapchainSupport()
{
    SwapchainSupportDetails details;
    const auto physDevice = m_Context.physicalDevice;

    details.capabilities = physDevice.getSurfaceCapabilitiesKHR(m_Context.surface);
    details.surfaceFormats = physDevice.getSurfaceFormatsKHR(m_Context.surface);
    details.presentModes = physDevice.getSurfacePresentModesKHR(m_Context.surface);

    return details;
}

vk::SurfaceFormatKHR VulkanSwapchain::ChooseSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    for (const auto format : availableFormats)
    {
        // XXX: Have a parameter passed in to choose desired formats instead of hardcoding them here.
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapchain::ChooseSwapchainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    for (const auto mode : availablePresentModes)
    {
        // XXX: Have parameter to choose present mode instead of hardcoding them here
        if (mode == vk::PresentModeKHR::eFifo)
            return mode;
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapchain::ChooseSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, u32 width, u32 height)
{
    constexpr auto undefined_size = std::numeric_limits<u32>::max();
    if (capabilities.currentExtent.width != undefined_size)
    {
        return capabilities.currentExtent;
    }

    vk::Extent2D extent;
    extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
    extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
    return extent;
}

void VulkanSwapchain::InitSemaphores()
{
    vk::SemaphoreCreateInfo createInfo{};
    m_PresentSemaphores.resize(m_ImageCount);

    for (size_t i = 0; i < m_ImageCount; i++)
    {
        VK_CHECK(m_Context.device.createSemaphore(&createInfo, nullptr, &m_PresentSemaphores[i]));
    }
}

} // namespace Vulkan

} // namespace SuohRHI