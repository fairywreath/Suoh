#pragma once

#include "VulkanInstance.h"
#include "VulkanSwapchain.h"

namespace SuohRHI
{

namespace Vulkan
{

struct VulkanDeviceContext
{
    vk::Queue graphicsQueue;
    i32 graphicsFamily{-1};
    vk::Queue presentQueue;
    i32 presentFamily{-1};
    vk::Queue computeQueue;
    i32 computeFamily{-1};

    vk::PhysicalDevice physicalDevice;
    vk::Device device;
};

/*
 * Device manager that handles Swapchain and multiple logical devices
 */
class VulkanDeviceManager
{
public:
    explicit VulkanDeviceManager(VulkanInstance& instance);
    ~VulkanDeviceManager();

    NON_COPYABLE(VulkanDeviceManager);
    NON_MOVEABLE(VulkanDeviceManager);

    [[nodiscard]] VulkanDeviceContext CreateDevice(const DeviceDesc& desc);

private:
    void GetPhysicalDevices();

    struct QueueFamilyIndices
    {
        u32 graphicsFamily;
        u32 presentFamily;
        u32 computeFamily;
    };

    /*
     * Picking Physical Device.
     */
    bool IsPhysicalDeviceSuitable(vk::PhysicalDevice device, const DeviceDesc&) const;
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, const DeviceDesc& desc) const;
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice device, const DeviceDesc& desc) const;

private:
    VulkanInstance& m_Instance;

    u32 m_PhysicalDeviceCount{0};
    std::vector<vk::PhysicalDevice> m_PhysicalDevices;

    // 1 surface for all (possible) devices.
    vk::SurfaceKHR m_Surface;
};

} // namespace Vulkan

} // namespace SuohRHI