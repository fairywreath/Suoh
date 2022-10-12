#include "VulkanDeviceManager.h"

#include <unordered_set>

// XXX: workaround for now since there is no instance/device manager RHI object.
#include <GLFW/glfw3.h>

namespace SuohRHI
{

namespace Vulkan
{

VulkanDeviceManager::VulkanDeviceManager(VulkanInstance& instance) : m_Instance(instance)
{
    GetPhysicalDevices();
}

VulkanDeviceManager::~VulkanDeviceManager()
{
    if (m_Surface)
    {
        m_Instance.GetVkInstance().destroySurfaceKHR(m_Surface);
    }
}

VulkanDeviceContext VulkanDeviceManager::CreateDevice(const DeviceDesc& desc)
{
    VulkanDeviceContext result;
    result.physicalDevice = nullptr;
    result.device = nullptr;

    if (!m_Surface)
    {
        // XXX: Get surface on first logical device creation for now
        if (glfwCreateWindowSurface(m_Instance.GetVkInstance(), (GLFWwindow*)desc.glfwWindowPtr, nullptr, (VkSurfaceKHR*)&m_Surface)
            != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create GLFW Window surface!");
            return result;
        }
    }

    /*
     * Pick physical device.
     */
    vk::PhysicalDevice physDevice = nullptr;
    for (const auto device : m_PhysicalDevices)
    {
        if (IsPhysicalDeviceSuitable(device, desc))
        {
            physDevice = device;
        }
    }

    if (!physDevice)
    {
        LOG_ERROR("No suitable physical device found!");
        return result;
    }

    auto properties = physDevice.getProperties();
    auto properties2 = physDevice.getProperties2();
    auto features = physDevice.getFeatures();

    // XXX: Check properties and features specified in description are supported by physical device.

    /*
     * Create Logical Device.
     */
    auto indices = FindQueueFamilies(physDevice, desc);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const std::unordered_set<u32> uniqueQueueFamilies = {indices.graphicsFamily, indices.computeFamily, indices.presentFamily};
    const float queuePriority = 1.0f;
    for (const auto queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo().setQueueFamilyIndex(queueFamily).setQueueCount(1).setPQueuePriorities(&queuePriority));
    }

    // XXX: Actually request features based on description and not hardcoded here?
    auto deviceFeatures = vk::PhysicalDeviceFeatures()
                              .setGeometryShader(true)
                              .setMultiDrawIndirect(true)
                              .setPipelineStatisticsQuery(true)
                              .setTessellationShader(true)
                              .setShaderSampledImageArrayDynamicIndexing(true);

    // Enable vulkan 1.1 features.
    auto vulkan11Features = vk::PhysicalDeviceVulkan11Features().setShaderDrawParameters(true);

    // Enable vulkan 1.2 features.
    auto vulkan12Features = vk::PhysicalDeviceVulkan12Features()
                                .setDescriptorIndexing(true)
                                .setRuntimeDescriptorArray(true)
                                .setDescriptorBindingPartiallyBound(true)
                                .setDescriptorBindingVariableDescriptorCount(true)
                                .setTimelineSemaphore(true)
                                .setShaderSampledImageArrayNonUniformIndexing(true)
                                .setPNext(&vulkan11Features);

    // XXX: Add device extensions specified in description?
    auto deviceExtensions = DEVICE_EXTENSIONS;

    // XXX: Add device layers specified in description?
    std::vector<const char*> deviceLayers;
    if (ENABLE_VALIDATION_LAYERS)
        deviceLayers.insert(std::end(deviceLayers), std::begin(VALIDATION_LAYERS), std::end(VALIDATION_LAYERS));

    auto deviceCreateInfo = vk::DeviceCreateInfo()
                                .setQueueCreateInfoCount(static_cast<u32>(queueCreateInfos.size()))
                                .setPQueueCreateInfos(queueCreateInfos.data())
                                .setEnabledExtensionCount(static_cast<u32>(deviceExtensions.size()))
                                .setPpEnabledExtensionNames(deviceExtensions.data())
                                .setPEnabledFeatures(&deviceFeatures)
                                .setEnabledLayerCount(static_cast<u32>(deviceLayers.size()))
                                .setPpEnabledLayerNames(deviceLayers.data())
                                .setPNext(&vulkan12Features);

    vk::Device device;
    const vk::Result res = physDevice.createDevice(&deviceCreateInfo, nullptr, &device);
    if (res != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create logical device with error vulkan error ", res, "!");
        return result;
    }

    device.getQueue(indices.graphicsFamily, 0, &result.graphicsQueue);
    device.getQueue(indices.presentFamily, 0, &result.presentQueue);
    device.getQueue(indices.computeFamily, 0, &result.computeQueue);

    result.graphicsFamily = indices.graphicsFamily;
    result.presentFamily = indices.presentFamily;
    result.computeFamily = indices.computeFamily;

    result.physicalDevice = physDevice;
    result.device = device;

    return result;
}

void VulkanDeviceManager::GetPhysicalDevices()
{
    auto instance = m_Instance.GetVkInstance();

    auto devices = instance.enumeratePhysicalDevices();

    for (auto device : devices)
    {
        m_PhysicalDevices.push_back(device);
    }

    m_PhysicalDeviceCount = m_PhysicalDevices.size();
}

bool VulkanDeviceManager::IsPhysicalDeviceSuitable(vk::PhysicalDevice device, const DeviceDesc& desc) const
{
    auto properties = device.getProperties();
    auto features = device.getFeatures();

    QueueFamilyIndices indices = FindQueueFamilies(device, desc);

    bool extensionsSupported = CheckDeviceExtensionSupport(device, desc);

    return (indices.graphicsFamily != -1 && indices.computeFamily != -1 && indices.presentFamily != -1) && extensionsSupported;
}

VulkanDeviceManager::QueueFamilyIndices VulkanDeviceManager::FindQueueFamilies(vk::PhysicalDevice device, const DeviceDesc& desc) const
{
    auto properties = device.getQueueFamilyProperties();

    QueueFamilyIndices indices;
    indices.graphicsFamily = -1;
    indices.computeFamily = -1;
    indices.presentFamily = -1;

    for (int i = 0; i < properties.size(); i++)
    {
        const auto& props = properties[i];

        // XXX: Need to deal with case where DIFFERENT queues are used for each functionality

        if (indices.graphicsFamily == -1)
        {
            if (props.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                indices.graphicsFamily = i;
            }
        }

        if (indices.computeFamily == -1)
        {
            if (props.queueFlags & vk::QueueFlagBits::eCompute)
            {
                indices.computeFamily = i;
            }
        }

        if (indices.presentFamily == -1)
        {
            if (device.getSurfaceSupportKHR(i, m_Surface))
            {
                indices.presentFamily = i;
            }
        }

        if (indices.graphicsFamily != -1 && indices.computeFamily != -1 && indices.presentFamily != -1)
            break;
    }

    return indices;
}
bool VulkanDeviceManager::CheckDeviceExtensionSupport(vk::PhysicalDevice device, const DeviceDesc& desc) const
{
    auto extensions = device.enumerateDeviceExtensionProperties();

    std::unordered_set<std::string> requiredExtensions(desc.deviceExtensions.begin(), desc.deviceExtensions.end());
    // Manually insert swapchain dependency for now
    requiredExtensions.insert(std::begin(DEVICE_EXTENSIONS), std::end(DEVICE_EXTENSIONS));

    for (const auto& extension : extensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

} // namespace Vulkan

} // namespace SuohRHI
