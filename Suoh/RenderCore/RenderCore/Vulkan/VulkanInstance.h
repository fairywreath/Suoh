#pragma once

#include "VulkanCommon.h"

namespace RenderCore
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

class VulkanInstance
{
public:
    explicit VulkanInstance(const DeviceDesc& desc);
    ~VulkanInstance();

    NON_COPYABLE(VulkanInstance);
    NON_MOVEABLE(VulkanInstance);

    [[nodiscard]] VulkanDeviceContext CreateDevice(const DeviceDesc& desc);

    [[nodiscard]] vk::Instance GetVkInstance() const
    {
        return m_Instance;
    }
    [[nodiscard]] vk::SurfaceKHR GetVkSurfaceKHR() const
    {
        return m_Surface;
    }

private:
    void InitInstance();
    void DestroyInstance();

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
    bool IsPhysicalDeviceSuitable(vk::PhysicalDevice device, const DeviceDesc& desc) const;
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, const DeviceDesc& desc) const;
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice device, const DeviceDesc& desc) const;

private:
    vk::DynamicLoader m_DynamicLoader;

    vk::Instance m_Instance;
    vk::DebugUtilsMessengerEXT m_DebugMessenger;

    vk::SurfaceKHR m_Surface;
    void* m_glfwWindowPtr;

    u32 m_PhysicalDeviceCount{0};
    std::vector<vk::PhysicalDevice> m_PhysicalDevices;
};

} // namespace Vulkan

} // namespace RenderCore
