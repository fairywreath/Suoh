#include "VKDevice.h"

#include <Logger.h>

#include <set>
#include <string>

namespace Suou
{

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices
{
    u32 graphicsFamily;
    u32 presentFamily;

    bool complete = false;
};

namespace
{

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    indices.graphicsFamily = -1;
    int i = 0;
    bool graphicsFound = false;
    bool presentFound = false;

    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            graphicsFound = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
            presentFound = true;
        }

        if (graphicsFound && presentFound)
        {
            indices.complete = true;
            break;
        }

        i++;
    }

    return indices;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static bool isPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    return indices.complete && extensionsSupported;
}

} // anonymous namespace

VKDevice::VKDevice(VkInstance instance, VkSurfaceKHR surface)
    : mInstance(instance), mSurface(surface)
{
    initPhysDevice();
    initLogicalDevice();
}

VKDevice::~VKDevice()
{
}

void VKDevice::destroy()
{
    vkDestroyDevice(mLogicalDevice, nullptr);
}

VkPhysicalDevice VKDevice::getPhysical() const
{
    return mPhysDevice;
}

const VkDevice& VKDevice::getLogical() const
{
    return mLogicalDevice;
}

VkQueue VKDevice::getGraphicsQueue() const
{
    return mGraphicsQueue;
}

u32 VKDevice::getGraphicsFamily() const
{
    return mGraphicsFamily;
}

VkQueue VKDevice::getPresentQueue() const
{
    return mPresentQueue;
}

u32 VKDevice::getPresentFamily() const
{
    return mPresentFamily;
}

const VkPhysicalDeviceProperties& VKDevice::getPhysDeviceProperties() const
{
    return mPhysDeviceProperties;
}

void VKDevice::initPhysDevice()
{
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isPhysicalDeviceSuitable(device, mSurface))
        {
            mPhysDevice = device;
            break;
        }
    }

    if (mPhysDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    vkGetPhysicalDeviceProperties(mPhysDevice, &mPhysDeviceProperties);

    vkGetPhysicalDeviceFeatures(mPhysDevice, &mPhysDeviceFeatures);
    if (!mPhysDeviceFeatures.geometryShader)
    {
        LOG_FATAL("Geometry shader not supported!");
        throw std::runtime_error("Geometry shader not supported!");
    }

    VkPhysicalDeviceVulkan11Features vk11Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    };

    mPhysDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    mPhysDeviceFeatures2.pNext = &vk11Features;
    vkGetPhysicalDeviceFeatures2(mPhysDevice, &mPhysDeviceFeatures2);

    if (!vk11Features.shaderDrawParameters)
    {
        LOG_FATAL("shaderDrawParameters not supported!");
        throw std::runtime_error("shaderDrawParameters not supported!");
    }
}

void VKDevice::initLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(mPhysDevice, mSurface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {
        .geometryShader = VK_TRUE,
        .pipelineStatisticsQuery = VK_TRUE,
    };

    // enable vk1.1 phys device features
    VkPhysicalDeviceVulkan11Features vk11Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .shaderDrawParameters = VK_TRUE,
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vk11Features,
    };

    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (VKCommon::EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<u32>(VKCommon::ValidationLayers.size());
        createInfo.ppEnabledLayerNames = VKCommon::ValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(mPhysDevice, &createInfo, nullptr, &mLogicalDevice));

    vkGetDeviceQueue(mLogicalDevice, indices.graphicsFamily, 0, &mGraphicsQueue);
    vkGetDeviceQueue(mLogicalDevice, indices.presentFamily, 0, &mPresentQueue);
    mGraphicsFamily = indices.graphicsFamily;
    mPresentFamily = indices.presentFamily;
}

} // namespace Suou
