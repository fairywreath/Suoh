#include "VKRenderDevice.h"

#include <GLFW/glfw3.h>

namespace Suou
{

namespace
{

static bool checkValidationLayerSupport()
{
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VKCommon::ValidationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static std::vector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (VKCommon::EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

[[nodiscard]] static VkInstance createVkInstance()
{
    if (VKCommon::EnableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Requested validation layers unavailable");
    }

    VkApplicationInfo appInfo
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Suou",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Suou Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };


    VkInstanceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo
    };

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (VKCommon::EnableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<u32>(VKCommon::ValidationLayers.size());
        createInfo.ppEnabledLayerNames = VKCommon::ValidationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    VkInstance instance;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
    return instance;
}

/* 
 * debug callback EXT utils
 */

static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}


static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
    {
        // XXX: use internal logging tool?
        std::cerr << "VK validation layer: " << pCallbackData->pMessage << std::endl;
    }


    return VK_FALSE;
}

static VkDebugUtilsMessengerEXT createVkDebugUtilsMessenger(VkInstance instance)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr
    };

    VkDebugUtilsMessengerEXT debugMessenger;
    VK_CHECK(createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
    return debugMessenger;
}

VkSurfaceKHR createVkSurfaceKHRFromGLFW(VkInstance instance, GLFWwindow* window)
{
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to craete window surface");
	}
	return surface;
}

} // anonymous namespace


VKRenderDevice::VKRenderDevice(Window* window) :
    mInstance(createVkInstance()),
    mDebugMessenger(createVkDebugUtilsMessenger(mInstance)),
    mSurface(createVkSurfaceKHRFromGLFW(mInstance, static_cast<GLFWwindow*>(window->getNativeWindow()))),
    mInitialized(false),
    mDevice(mInstance, mSurface),
    mSwapchain(mSurface, mDevice, window->getWidth(), window->getHeight())
{
    init();
}

VKRenderDevice::~VKRenderDevice()
{
    destroy();
}

void VKRenderDevice::init()
{
    initCommands();
}

void VKRenderDevice::initCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = mDevice.getGraphicsFamily()
    };

    VK_CHECK(vkCreateCommandPool(mDevice.getLogical(), &commandPoolInfo, nullptr, &mCommandPool));
}

void VKRenderDevice::destroy()
{
    auto device = mDevice.getLogical();

    vkDestroyCommandPool(device, mCommandPool, nullptr);

    mSwapchain.destroy();
    mDevice.destroy();

    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    if (VKCommon::EnableValidationLayers)
    {
        destroyDebugUtilsMessengerEXT(mInstance ,mDebugMessenger, nullptr);
    }
    vkDestroyInstance(mInstance, nullptr);
}

} // Suou
