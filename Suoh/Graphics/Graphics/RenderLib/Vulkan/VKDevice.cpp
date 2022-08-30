#include "VKDevice.h"

#include <Core/Logger.h>

#include <set>
#include <string>

namespace Suoh
{

namespace
{

const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices
{
    u32 graphicsFamily{};
    u32 presentFamily{};
    u32 computeFamily{};

    bool complete{false};
};

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
    bool computeFound = false;
    bool presentFound = false;

    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
            graphicsFound = true;
        }

        // XXX: need to deal with case when graphics and compute families are different
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.computeFamily = i;
            computeFound = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
            presentFound = true;
        }

        if (graphicsFound && presentFound && computeFound)
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

VkQueue VKDevice::getComputeQueue() const
{
    return mComputeQueue;
}

u32 VKDevice::getComputeFamily() const
{
    return mComputeFamily;
}

bool VKDevice::computeCapable() const
{
    return mUseCompute;
}

VkQueue VKDevice::getPresentQueue() const
{
    return mPresentQueue;
}

u32 VKDevice::getPresentFamily() const
{
    return mPresentFamily;
}

u32 VKDevice::getQueueFamilyIndicesCount() const
{
    return mQueueFamilyIndicesCount;
}

std::vector<u32> VKDevice::getQueueFamilyIndices() const
{
    std::set<u32> indices{mGraphicsFamily, mComputeFamily, mPresentFamily};

    return std::vector<u32>(indices.begin(), indices.end());
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
    std::set<u32> uniqueQueueFamilies = {indices.graphicsFamily, indices.computeFamily, indices.presentFamily};

    mQueueFamilyIndicesCount = uniqueQueueFamilies.size();

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
        .multiDrawIndirect = VK_TRUE,
        .pipelineStatisticsQuery = VK_TRUE,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
    };

    // enable descriptor indexing for texture array
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };

    // enable vk1.1 phys device features
    VkPhysicalDeviceVulkan11Features vk11Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &physicalDeviceDescriptorIndexingFeatures, // link to descriptor indexing feature
        .shaderDrawParameters = VK_TRUE,
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vk11Features,
        .queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

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
    vkGetDeviceQueue(mLogicalDevice, indices.computeFamily, 0, &mComputeQueue);
    vkGetDeviceQueue(mLogicalDevice, indices.presentFamily, 0, &mPresentQueue);
    mGraphicsFamily = indices.graphicsFamily;
    mComputeFamily = indices.computeFamily;
    mPresentFamily = indices.presentFamily;

    // XXX: always assume compute is supported for now
    mUseCompute = true;
} // namespace Suoh

} // namespace Suoh
