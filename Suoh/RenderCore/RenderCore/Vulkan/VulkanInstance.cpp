#include "VulkanInstance.h"

#include <GLFW/glfw3.h>

#include <unordered_set>

// Default Disptach Dynamic Loader for Vulkan.hpp
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace RenderCore
{

namespace Vulkan
{

namespace
{

bool CheckLayerSupport()
{
    u32 layerCount;
    VK_CHECK_ABORT(vk::enumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    VK_CHECK_ABORT(vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    for (const char* layerName : VALIDATION_LAYERS)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> GetRequiredExtensions()
{
    u32 glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DEBUG_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_ERROR("VK Validation Layer: ", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

} // namespace

VulkanInstance::VulkanInstance(const DeviceDesc& desc) : m_glfwWindowPtr(desc.glfwWindowPtr)
{
    InitInstance();
    GetPhysicalDevices();
}

VulkanInstance::~VulkanInstance()
{
    DestroyInstance();
}

VulkanDeviceContext VulkanInstance::CreateDevice(const DeviceDesc& desc)
{
    VulkanDeviceContext result;
    result.physicalDevice = nullptr;
    result.device = nullptr;

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

    // auto properties = physDevice.getProperties();
    // auto properties2 = physDevice.getProperties2();
    // auto features = physDevice.getFeatures();

    // XXX: Check properties and features specified in description are supported by physical device.

    /*
     * Create Logical Device.
     */
    auto indices = FindQueueFamilies(physDevice, desc);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const std::unordered_set<u32> uniqueQueueFamilies
        = {indices.graphicsFamily, indices.computeFamily, indices.presentFamily};
    const float queuePriority = 1.0f;
    for (const auto queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo()
                                       .setQueueFamilyIndex(queueFamily)
                                       .setQueueCount(1)
                                       .setPQueuePriorities(&queuePriority));
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
        deviceLayers.insert(std::end(deviceLayers), std::begin(VALIDATION_LAYERS),
                            std::end(VALIDATION_LAYERS));

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

void VulkanInstance::InitInstance()
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr
        = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if (ENABLE_VALIDATION_LAYERS && !CheckLayerSupport())
    {
        LOG_ERROR("Vulkan validation layer not supported!");
        return;
    }

    /*
     * Instance creation.
     */
    std::vector<const char*> layers;
    if (ENABLE_VALIDATION_LAYERS)
    {
        layers.insert(std::end(layers), std::begin(VALIDATION_LAYERS), std::end(VALIDATION_LAYERS));
    }
    auto extensions = GetRequiredExtensions();

    vk::ApplicationInfo appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);
    vk::InstanceCreateInfo instanceInfo = vk::InstanceCreateInfo()
                                              .setPApplicationInfo(&appInfo)
                                              .setEnabledExtensionCount(extensions.size())
                                              .setPpEnabledExtensionNames(extensions.data())
                                              .setEnabledLayerCount(layers.size())
                                              .setPpEnabledLayerNames(layers.data());

    m_Instance = vk::createInstance(instanceInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);

    /*
     * Debug Messenger creation.
     */
    auto severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                                         .setMessageSeverity(severityFlags)
                                                         .setMessageType(typeFlags)
                                                         .setPfnUserCallback(DEBUG_CALLBACK);
    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugInfo);

    /*
     * Surface creation.
     */
    if (glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_glfwWindowPtr, nullptr,
                                (VkSurfaceKHR*)&m_Surface)
        != VK_SUCCESS)
    {
        LOG_ERROR("VulkanInstance: Failed to create VkSurfaceKHR!");
        return;
    }
}

void VulkanInstance::DestroyInstance()
{
    m_Instance.destroySurfaceKHR(m_Surface);
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger);
    m_Instance.destroy();
}

void VulkanInstance::GetPhysicalDevices()
{
    m_PhysicalDevices = m_Instance.enumeratePhysicalDevices();
    m_PhysicalDeviceCount = m_PhysicalDevices.size();
}

bool VulkanInstance::IsPhysicalDeviceSuitable(vk::PhysicalDevice device,
                                              const DeviceDesc& desc) const
{
    // auto properties = device.getProperties();
    // auto features = device.getFeatures();

    QueueFamilyIndices indices = FindQueueFamilies(device, desc);

    bool extensionsSupported = CheckDeviceExtensionSupport(device, desc);

    return (indices.graphicsFamily != -1 && indices.computeFamily != -1
            && indices.presentFamily != -1)
           && extensionsSupported;
}

VulkanInstance::QueueFamilyIndices VulkanInstance::FindQueueFamilies(vk::PhysicalDevice device,
                                                                     const DeviceDesc& desc) const
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

        if (indices.graphicsFamily != -1 && indices.computeFamily != -1
            && indices.presentFamily != -1)
            break;
    }

    return indices;
}

bool VulkanInstance::CheckDeviceExtensionSupport(vk::PhysicalDevice device,
                                                 const DeviceDesc& desc) const
{
    auto extensions = device.enumerateDeviceExtensionProperties();

    std::unordered_set<std::string> requiredExtensions(desc.deviceExtensions.begin(),
                                                       desc.deviceExtensions.end());

    requiredExtensions.insert(std::begin(DEVICE_EXTENSIONS), std::end(DEVICE_EXTENSIONS));

    for (const auto& extension : extensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

} // namespace Vulkan

} // namespace RenderCore
