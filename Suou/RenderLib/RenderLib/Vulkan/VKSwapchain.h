#pragma once

#include "VKDevice.h"

namespace Suou
{

class VKSwapchain
{
public:
    VKSwapchain(VkSurfaceKHR surface, const VKDevice& device, u32 width, u32 height);
    ~VKSwapchain();

    void destroy();

    void acquireNextImage();
    void present(VkSemaphore renderSemaphore);

    VkImage getImage(std::size_t index) const;
    const VkImageView& getImageView(std::size_t index) const;

    size_t getImageCount() const;
    size_t getImageIndex() const;
    const VkSemaphore& getCurrentPresentSemaphore() const;

    VkFormat getSwapchainImageFormat() const;
    VkExtent2D getExtent() const;

private:
    void initSwapchain(VkSurfaceKHR surface, const VKDevice& device, u32 width, u32 height);
    void initSemaphores();

private:
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapchain;

    const VKDevice& mDevice;

    VkFormat mSwapchainImageFormat{};
    VkPresentModeKHR mPresentMode{};
    VkExtent2D mExtent{};

    std::vector<VkImage> mImages;
    std::vector<VkImageView> mImageViews;

    // swapchain semaphore, waits for swapchain image to become available
    VkSemaphore mSwapchainSemaphore;

    // render semaphore
    std::vector<VkSemaphore> mPresentSemaphores;

    size_t mImageCount = 0;
    u32 mImageIndex = 0;
};

} // namespace Suou
