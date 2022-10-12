#include "VulkanInstance.h"

#include <GLFW/glfw3.h>

// Default Disptach Dynamic Loader for Vulkan.hpp
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace SuohRHI
{

namespace Vulkan
{

namespace
{

bool CheckLayerSupport()
{
    u32 layerCount;
    VK_CHECK(vk::enumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<vk::LayerProperties> availableLayers(layerCount);
    VK_CHECK(vk::enumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

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
    // XXX: Put here for now
    u32 glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ENABLE_VALIDATION_LAYERS)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DEBUG_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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

VulkanInstance::VulkanInstance()
{
    InitInstance();
}

VulkanInstance::~VulkanInstance()
{
    DestroyInstance();
}

vk::Instance VulkanInstance::GetVkInstance() const
{
    return m_Instance;
}

void VulkanInstance::InitInstance()
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
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
    auto severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    auto typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                     | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                                         .setMessageSeverity(severityFlags)
                                                         .setMessageType(typeFlags)
                                                         .setPfnUserCallback(DEBUG_CALLBACK);
    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugInfo);
}

void VulkanInstance::DestroyInstance()
{
    m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger);
    m_Instance.destroy();
}

} // namespace Vulkan

} // namespace SuohRHI
