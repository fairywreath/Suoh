#include "VKSwapchain.h"

#include <array>

#include <Asserts.h>

namespace Suou
{

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

namespace
{

static SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

static VkSurfaceFormatKHR chooseSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

static VkPresentModeKHR chooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height)
{
    constexpr auto undefined_size = std::numeric_limits<u32>::max();
    if (capabilities.currentExtent.width != undefined_size)
    {
        return capabilities.currentExtent;
    }

    VkExtent2D extent;
    extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
    extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
    return extent;
}

} // anonymous namespace

VKSwapchain::VKSwapchain(VkSurfaceKHR surface, const VKDevice& device, u32 width, u32 height)
    : mDevice(device)
{
    initSwapchain(surface, device, width, height);
    initSemaphores();
}

VKSwapchain::~VKSwapchain()
{
}

void VKSwapchain::destroy()
{
    auto device = mDevice.getLogical();

    for (auto semaphore : mPresentSemaphores)
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }

    vkDestroySemaphore(device, mSwapchainSemaphore, nullptr);

    for (auto imageView : mImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, mSwapchain, nullptr);
}

void VKSwapchain::acquireNextImage()
{
    // XXX: change to match swapchain return codes
    VK_CHECK(vkAcquireNextImageKHR(mDevice.getLogical(), mSwapchain, 1000000000, mPresentSemaphores[mImageIndex],
                                   nullptr, &mImageIndex));
}

void VKSwapchain::present(VkSemaphore renderSemaphore)
{
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain,
        .pImageIndices = &mImageIndex,
    };

    VK_CHECK(vkQueuePresentKHR(mDevice.getPresentQueue(), &presentInfo));

    mImageIndex = (mImageIndex + 1) % mImageCount;
}

VkImage VKSwapchain::getImage(std::size_t index) const
{
    SUOU_ASSERT(index < mImageCount);

    return mImages[index];
}

const VkImageView& VKSwapchain::getImageView(std::size_t index) const
{
    SUOU_ASSERT(index < mImageCount);

    return mImageViews[index];
}

VkFormat VKSwapchain::getSwapchainImageFormat() const
{
    return mSwapchainImageFormat;
}

VkExtent2D VKSwapchain::getExtent() const
{
    return mExtent;
}

size_t VKSwapchain::getImageCount() const
{
    return mImageCount;
}

size_t VKSwapchain::getImageIndex() const
{
    return mImageIndex;
}

const VkSemaphore& VKSwapchain::getCurrentPresentSemaphore() const
{
    return mPresentSemaphores[mImageIndex];
}

void VKSwapchain::initSwapchain(VkSurfaceKHR surface, const VKDevice& device, u32 width, u32 height)
{
    auto physDevice = device.getPhysical();
    auto logicalDevice = device.getLogical();

    SwapchainSupportDetails swapChainSupport = querySwapChainSupport(physDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapchainSurfaceFormat(swapChainSupport.formats);
    mSwapchainImageFormat = surfaceFormat.format;
    mPresentMode = chooseSwapchainPresentMode(swapChainSupport.presentModes);
    mExtent = chooseSwapchainExtent(swapChainSupport.capabilities, width, height);

    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = mExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 graphicsFamily = mDevice.getGraphicsFamily();
    u32 presentFamily = mDevice.getPresentFamily();
    const std::array<u32, 2> queueFamilyIndices = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &mSwapchain));

    // init images
    mImages.resize(imageCount);
    mImageCount = imageCount;
    vkGetSwapchainImagesKHR(logicalDevice, mSwapchain, &imageCount, mImages.data());

    // init imageviews
    mImageViews.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = mImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = mSwapchainImageFormat,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        VK_CHECK(vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &mImageViews[i]));
    }
}

void VKSwapchain::initSemaphores()
{
    VkDevice device = mDevice.getLogical();
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
    };

    VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &mSwapchainSemaphore));

    mPresentSemaphores.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++)
    {
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &mPresentSemaphores[i]));
    }
}

} // namespace Suou
