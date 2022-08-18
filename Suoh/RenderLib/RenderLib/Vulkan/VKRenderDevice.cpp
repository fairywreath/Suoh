#include "VKRenderDevice.h"

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/version.h>

#include <StandAlone/ResourceLimits.h>
#include <glslang/Include/glslang_c_interface.h>

#include <Asserts.h>
#include <Logger.h>

#include <array>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace Suoh
{

namespace
{

static bool checkValidationLayerSupport()
{
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VKCommon::ValidationLayers)
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

static std::vector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (VKCommon::EnableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

[[nodiscard]] static VkInstance createVkInstance()
{
    if (VKCommon::EnableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Requested validation layers unavailable");
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Suoh",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Suoh Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &appInfo};

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (VKCommon::EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<u32>(VKCommon::ValidationLayers.size());
        createInfo.ppEnabledLayerNames = VKCommon::ValidationLayers.data();
    }
    else
    {
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
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };

    VkDebugUtilsMessengerEXT debugMessenger;
    VK_CHECK(createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger));
    return debugMessenger;
}

VkSurfaceKHR createVkSurfaceKHRFromGLFW(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface");
    }
    return surface;
}

static VkResult setDebugUtilsObjectNameEXT(VkInstance instance, VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    if (func != nullptr)
    {
        return func(device, pNameInfo);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

u32 bytesPerTexFormat(VkFormat fmt)
{
    switch (fmt)
    {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R16_SFLOAT:
        return 2;
    case VK_FORMAT_R16G16_SFLOAT:
        return 4;
    case VK_FORMAT_R16G16_SNORM:
        return 4;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return 4;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 4 * sizeof(uint16_t);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * sizeof(float);
    default:
        break;
    }
    return 0;
}

} // anonymous namespace

VKRenderDevice::VKRenderDevice(Window* window)
    : mInstance(createVkInstance()), mDebugMessenger(createVkDebugUtilsMessenger(mInstance)),
      mSurface(createVkSurfaceKHRFromGLFW(mInstance, static_cast<GLFWwindow*>(window->getNativeWindow()))),
      mInitialized(false), mDevice(mInstance, mSurface),
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
    glslang_initialize_process();

    initAllocator();
    initCommands();
    initSynchronizationObjects();
}

void VKRenderDevice::initCommands()
{
    VkCommandPoolCreateInfo commandPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = mDevice.getGraphicsFamily(),
    };

    VK_CHECK(vkCreateCommandPool(mDevice.getLogical(), &commandPoolInfo, nullptr, &mCommandPool));

    mCommandBuffers.resize(mSwapchain.getImageCount());

    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = mCommandPool, // XXX: use different command pool?
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (u32)mSwapchain.getImageCount(),
    };

    VK_CHECK(vkAllocateCommandBuffers(mDevice.getLogical(), &allocInfo, mCommandBuffers.data()));
}

void VKRenderDevice::initAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = mDevice.getPhysical(),
        .device = mDevice.getLogical(),
        .instance = mInstance,
    };

    vmaCreateAllocator(&allocatorInfo, &mAllocator);
}
void VKRenderDevice::initSynchronizationObjects()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
    };

    VK_CHECK(vkCreateSemaphore(mDevice.getLogical(), &semaphoreCreateInfo, nullptr, &mRenderSemaphore));
}

void VKRenderDevice::destroy()
{
    auto device = mDevice.getLogical();

    vkFreeCommandBuffers(mDevice.getLogical(), mCommandPool, mCommandBuffers.size(), mCommandBuffers.data());

    vkDestroySemaphore(device, mRenderSemaphore, nullptr);

    vkDestroyCommandPool(device, mCommandPool, nullptr);
    vmaDestroyAllocator(mAllocator);

    mSwapchain.destroy();
    mDevice.destroy();

    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    if (VKCommon::EnableValidationLayers)
    {
        destroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
    }
    vkDestroyInstance(mInstance, nullptr);

    glslang_finalize_process();
}

VkCommandBuffer VKRenderDevice::beginSingleTimeCommands()
{
    VkCommandBuffer commandBuffer;

    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = mCommandPool, // XXX: use different command pool?
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    vkAllocateCommandBuffers(mDevice.getLogical(), &allocInfo, &commandBuffer);

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VKRenderDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{

    vkEndCommandBuffer(commandBuffer);

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    auto graphicsQueue = mDevice.getGraphicsQueue();
    auto& device = mDevice.getLogical();

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, mCommandPool, 1, &commandBuffer);
}

bool VKRenderDevice::setVkObjectName(void* object, VkObjectType objectType, const std::string& name)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = objectType,
        .objectHandle = (u64)object,
        .pObjectName = name.c_str()};

    return (setDebugUtilsObjectNameEXT(mInstance, mDevice.getLogical(), &nameInfo) == VK_SUCCESS);
}

const VkCommandBuffer& VKRenderDevice::getCurrentCommandBuffer()
{
    return mCommandBuffers[mSwapchain.getImageIndex()];
}

void VKRenderDevice::resetCommandPool()
{
    vkResetCommandPool(mDevice.getLogical(), mCommandPool, 0);
}

bool VKRenderDevice::submit(const VkCommandBuffer& commandBuffer)
{
    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; // or even VERTEX_SHADER_STAGE

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.getCurrentPresentSemaphore(),
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mRenderSemaphore,
    };

    VK_CHECK(vkQueueSubmit(mDevice.getGraphicsQueue(), 1, &submitInfo, nullptr));

    return true;
}

bool VKRenderDevice::present()
{
    mSwapchain.present(mRenderSemaphore);

    return true;
}

bool VKRenderDevice::deviceWaitIdle()
{
    VK_CHECK(vkDeviceWaitIdle(mDevice.getLogical()));

    return true;
}

void VKRenderDevice::swapchainAcquireNextImage()
{
    mSwapchain.acquireNextImage();
}

size_t VKRenderDevice::getSwapchainImageIndex()
{
    return mSwapchain.getImageIndex();
}

size_t VKRenderDevice::getSwapchainImageCount()
{
    return mSwapchain.getImageCount();
}

bool VKRenderDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, Buffer& buffer)
{
    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    const VmaAllocationCreateInfo allocInfo = {
        .usage = memUsage,
    };

    VK_CHECK(vmaCreateBuffer(mAllocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation,
                             nullptr));

    return true;
}

static int roundUpClosest(int numToRound, int multiple)
{
    if (multiple == 0)
    {
        return numToRound;
    }

    int roundDown = ((int)numToRound / multiple) * multiple;
    int roundUp = roundDown + multiple;

    return roundUp;
}

bool VKRenderDevice::createTexturedVertexBuffer(const std::string& filePath, Buffer& buffer, size_t& vertexBufferSize, size_t& indexBufferSize, size_t& indexBufferOffset)
{
    const aiScene* scene = aiImportFile(filePath.c_str(), aiProcess_Triangulate);

    if (!scene || !scene->HasMeshes())
    {
        LOG_ERROR("Failed to load model: ", filePath);
        return false;
    }

    const aiMesh* mesh = scene->mMeshes[0];
    struct VertexData
    {
        vec3 pos;
        vec2 texCoords;
    };

    std::vector<VertexData> vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D v = mesh->mVertices[i];
        aiVector3D t = mesh->mTextureCoords[0][i];
        vertices.push_back({
            .pos = vec3(v.x, v.z, v.y),
            .texCoords = vec2(t.x, t.y),
        });
    }

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        for (unsigned int j = 0; j < 3; j++)
        {
            indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }

    aiReleaseImport(scene);

    vertexBufferSize = sizeof(VertexData) * vertices.size();
    indexBufferSize = sizeof(unsigned int) * indices.size();

    VkDeviceSize minStorageBufferOffset = mDevice.getPhysDeviceProperties().limits.minStorageBufferOffsetAlignment;
    indexBufferOffset = roundUpClosest(vertexBufferSize, minStorageBufferOffset);

    VkDeviceSize bufferSize = indexBufferOffset + indexBufferSize;

    Buffer stagingBuffer;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer);

    void* data;
    mapMemory(stagingBuffer.allocation, &data);
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((u8*)data + indexBufferOffset, indices.data(), indexBufferSize);
    unmapMemory(stagingBuffer.allocation);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY, buffer);
    copyBuffer(stagingBuffer.buffer, buffer.buffer, bufferSize);

    destroyBuffer(stagingBuffer);

    return true;
}

void VKRenderDevice::destroyBuffer(Buffer& buffer)
{
    vmaDestroyBuffer(mAllocator, buffer.buffer, buffer.allocation);
}

void VKRenderDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    const VkBufferCopy copyParam = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyParam);

    endSingleTimeCommands(commandBuffer);
}
void VKRenderDevice::mapMemory(VmaAllocation allocation, void** data)
{
    vmaMapMemory(mAllocator, allocation, data);
}

void VKRenderDevice::unmapMemory(VmaAllocation allocation)
{
    vmaUnmapMemory(mAllocator, allocation);
}

void VKRenderDevice::uploadBufferData(Buffer& buffer, const void* data, const size_t dataSize)
{
    void* mappedData = nullptr;
    vmaMapMemory(mAllocator, buffer.allocation, &mappedData);
    memcpy(mappedData, data, dataSize);
    vmaUnmapMemory(mAllocator, buffer.allocation);
}

bool VKRenderDevice::createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                 VmaMemoryUsage memUsage, Image& image, VkImageCreateFlags flags)
{
    // const VkImageCreateInfo imageInfo = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //     .pNext = nullptr,
    //     .flags = flags,
    //     .imageType = VK_IMAGE_TYPE_2D,
    //     .format = format,
    //     .extent = VkExtent3D{.width = width, .height = height, .depth = 1},
    //     .mipLevels = 1,
    //     .arrayLayers = 1,
    //     .samples = VK_SAMPLE_COUNT_1_BIT,
    //     .tiling = tiling,
    //     .usage = usage,
    //     .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    //     .queueFamilyIndexCount = 0,
    //     .pQueueFamilyIndices = nullptr,
    //     .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    // };

    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D{.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = (u32)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = memUsage,
    };

    // if (vmaCreateImage(mAllocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr) != VK_SUCCESS)
    // {
    //     LOG_ERROR("Image creation faield!");
    // }

    VK_CHECK(vmaCreateImage(mAllocator, &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    return true;
}

bool VKRenderDevice::createImageView(VkFormat format, VkImageAspectFlags aspectFlags, Image& image)
{
    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};

    VK_CHECK(vkCreateImageView(mDevice.getLogical(), &viewInfo, nullptr, &image.imageView));

    return true;
}

bool VKRenderDevice::createTextureSampler(Texture& texture)
{
    const VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    VK_CHECK(vkCreateSampler(mDevice.getLogical(), &samplerInfo, nullptr, &texture.sampler));

    return true;
}

void VKRenderDevice::copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height, u32 layerCount)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = layerCount,
        },
        .imageOffset = VkOffset3D{.x = 0, .y = 0, .z = 0},
        .imageExtent = VkExtent3D{.width = width, .height = height, .depth = 1},
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VKRenderDevice::destroyImage(Image& image)
{
    auto device = mDevice.getLogical();
    vkDestroyImageView(device, image.imageView, nullptr);
    vmaDestroyImage(mAllocator, image.image, image.allocation);
}

void VKRenderDevice::destroyTexture(Texture& texture)
{
    auto device = mDevice.getLogical();
    vkDestroySampler(device, texture.sampler, nullptr);
    destroyImage(texture.image);
}

void VKRenderDevice::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 layerCount, u32 mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    transitionImageLayourCmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);
    endSingleTimeCommands(commandBuffer);
}

void VKRenderDevice::transitionImageLayourCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
                                              VkImageLayout newLayout, u32 layerCount, u32 mipLevels)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = layerCount}};

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || (format == VK_FORMAT_D16_UNORM) || (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT))
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    /* Convert back from read-only to updateable */
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    /* Convert from updateable texture to shader read-only */
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    /* Convert depth texture from undefined state to depth-stencil buffer */
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    /* Wait for render pass to complete */
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = 0;
        /*
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        ///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        */
        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    /* Convert back from read-only to color attachment */
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    /* Convert from updateable texture to shader read-only */
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    /* Convert back from read-only to depth attachment */
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    /* Convert from updateable depth texture to shader read-only */
    else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

bool VKRenderDevice::hasStencilComponent(VkFormat format)
{
    if ((format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT))
    {
        return true;
    }

    return false;
}

VkFormat VKRenderDevice::findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    bool isLinear = (tiling == VK_IMAGE_TILING_LINEAR);
    bool isOptimal = (tiling == VK_IMAGE_TILING_OPTIMAL);

    auto device = mDevice.getPhysical();

    for (auto format : formats)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (isLinear && ((props.linearTilingFeatures & features) == features))
        {
            return format;
        }

        if (isOptimal && ((props.optimalTilingFeatures & features) == features))
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VkFormat VKRenderDevice::findDepthFormat()
{
    return findSupportedFormat({
                                   VK_FORMAT_D32_SFLOAT,
                                   VK_FORMAT_D32_SFLOAT_S8_UINT,
                                   VK_FORMAT_D24_UNORM_S8_UINT,
                               },
                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VKRenderDevice::createDepthResources(u32 width, u32 height, Image& depthImage)
{
    VkFormat depthFormat = findDepthFormat();
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        LOG_ERROR("VKRenderDevice - Unabled to find depth format!");
        SUOH_ASSERT("Fatal: failed to find depth format!");
        return;
    }

    createImage(width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                depthImage);

    createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImage);

    transitionImageLayout(depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
}

bool VKRenderDevice::createTextureImage(const std::string& filePath, Texture& texture)
{
    int texWidth;
    int texHeight;
    int texChannels;

    stbi_uc* pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        LOG_ERROR("Failed to load texture image: ", filePath);
        return false;
    }

    const auto format = VK_FORMAT_R8G8B8A8_UNORM;

    createImage(texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, texture.image);
    updateTextureImage(texture.image, texWidth, texHeight, format, pixels);

    stbi_image_free(pixels);

    return true;
}

bool VKRenderDevice::createTextureImageFromData(const void* data, u32 width, u32 height, VkFormat format, Image& image, u32 layerCount, VkImageCreateFlags createFlags)
{
    createImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, image, createFlags);

    return updateTextureImage(image, width, height, format, data, layerCount);
}

bool VKRenderDevice::updateTextureImage(Image& image, u32 width, u32 height, VkFormat format, const void* imageData, u32 layerCount, VkImageLayout sourceImageLayout)
{
    u32 bytesPerPixel = bytesPerTexFormat(format);

    VkDeviceSize layerSize = width * height * bytesPerPixel;
    VkDeviceSize imageSize = layerSize * layerCount;

    Buffer stagingBuffer;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

    uploadBufferData(stagingBuffer, imageData, imageSize);

    transitionImageLayout(image.image, format, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, 1);
    copyBufferToImage(stagingBuffer.buffer, image.image, static_cast<u32>(width), static_cast<u32>(height), layerCount);
    transitionImageLayout(image.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

    destroyBuffer(stagingBuffer);

    return true;
}

bool VKRenderDevice::createDescriptorPool(u32 uniformBufferCount, u32 storageBufferCount, u32 samplerCount, VkDescriptorPool& descriptorPool)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    u32 imageCount = mSwapchain.getImageCount();

    if (uniformBufferCount > 0)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = imageCount * uniformBufferCount,
        });
    }

    if (storageBufferCount > 0)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = imageCount * storageBufferCount,
        });
    }

    if (samplerCount > 0)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = imageCount * samplerCount,
        });
    }

    const VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = (u32)imageCount,
        .poolSizeCount = (u32)poolSizes.size(),
        .pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(mDevice.getLogical(), &descPoolInfo, nullptr, &descriptorPool));

    return true;
}

bool VKRenderDevice::createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout& layout)
{
    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (u32)bindings.size(),
        .pBindings = bindings.data(),
    };

    VK_CHECK(vkCreateDescriptorSetLayout(mDevice.getLogical(), &layoutInfo, nullptr, &layout));

    return true;
}

bool VKRenderDevice::allocateDescriptorSets(VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& layouts, std::vector<VkDescriptorSet>& descriptorSets)
{
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = (u32)mSwapchain.getImageCount(),
        .pSetLayouts = layouts.data(),
    };

    descriptorSets.resize(mSwapchain.getImageCount());

    VK_CHECK(vkAllocateDescriptorSets(mDevice.getLogical(), &allocInfo, descriptorSets.data()));

    return true;
}

void VKRenderDevice::updateDescriptorSets(u32 descriptorWriteCount, const std::vector<VkWriteDescriptorSet>& descriptorWrites)
{
    vkUpdateDescriptorSets(mDevice.getLogical(), descriptorWriteCount, descriptorWrites.data(), 0, nullptr);
}

void VKRenderDevice::destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorLayout)
{
    vkDestroyDescriptorSetLayout(mDevice.getLogical(), descriptorLayout, nullptr);
}

void VKRenderDevice::destroyDescriptorPool(VkDescriptorPool descriptorPool)
{
    vkDestroyDescriptorPool(mDevice.getLogical(), descriptorPool, nullptr);
}

/*
 * Shaders implementations
 */
static std::string readShaderFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    fs::path path(filePath);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to read shader file: ", fs::absolute(path));
        return std::string();
    }

    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    while (code.find("#include ") != code.npos)
    {
        const auto pos = code.find("#include ");
        const auto p1 = code.find('<', pos);
        const auto p2 = code.find('>', pos);
        if (p1 == code.npos || p2 == code.npos || p2 <= p1)
        {
            LOG_ERROR("Failed loading shader program: ", code);
            file.close();
            return std::string();
        }
        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
        const std::string include = readShaderFile(name.c_str());
        code.replace(pos, p2 - pos + 1, include.c_str());
    }

    file.close();
    return code;
}

static void printShaderSource(const std::string& source)
{
    if (source.empty())
    {
        return;
    }

    int lineNum = 1;
    std::string line = std::to_string(lineNum) + " ";

    for (const auto& c : source)
    {
        if (c == '\n')
        {
            LOG_DEBUG(line);
            line = std::to_string(++lineNum) + " ";
        }
        else if (c == '\r')
        {
        }
        else
        {
            line += c;
        }
    }

    if (!line.empty())
    {
        LOG_DEBUG(line);
    }
}

static glslang_stage_t glslangShaderStageFromFileName(const std::string& filePath)
{
    if (filePath.ends_with(".vert"))
        return GLSLANG_STAGE_VERTEX;

    if (filePath.ends_with(".frag"))
        return GLSLANG_STAGE_FRAGMENT;

    if (filePath.ends_with(".geom"))
        return GLSLANG_STAGE_GEOMETRY;

    if (filePath.ends_with(".comp"))
        return GLSLANG_STAGE_COMPUTE;

    if (filePath.ends_with(".tesc"))
        return GLSLANG_STAGE_TESSCONTROL;

    if (filePath.ends_with(".tese"))
        return GLSLANG_STAGE_TESSEVALUATION;

    return GLSLANG_STAGE_VERTEX;
}

static size_t compileShader(const std::string& shaderSource, glslang_stage_t stage, ShaderBinary& SPIRV)
{
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_1,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = shaderSource.c_str(),
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = (const glslang_resource_t*)&glslang::DefaultTBuiltInResource,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    // XXX: need to redirect these logs to the internal logging library

    if (!glslang_shader_preprocess(shader, &input))
    {
        LOG_ERROR("GLSL preprocessing failed");
        LOG_ERROR(glslang_shader_get_info_log(shader));
        LOG_ERROR(glslang_shader_get_info_debug_log(shader));
        LOG_ERROR("Source: ");

        // LOG_ERROR(input.code);
        printShaderSource(shaderSource);
        return 0;
    }

    if (!glslang_shader_parse(shader, &input))
    {
        LOG_ERROR("GLSL parsing failed");
        LOG_ERROR("\n", glslang_shader_get_info_log(shader),
                  glslang_shader_get_info_debug_log(shader));

        // LOG_ERROR(glslang_shader_get_preprocessed_code(shader));
        printShaderSource(shaderSource);
        return 0;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    int msgs = GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT;

    if (!glslang_program_link(program, msgs))
    {
        LOG_ERROR("GLSL linking failed");
        LOG_ERROR(glslang_program_get_info_log(program));
        LOG_ERROR(glslang_program_get_info_debug_log(program));
        return 0;
    }

    glslang_program_SPIRV_generate(program, stage);

    SPIRV.resize(glslang_program_SPIRV_get_size(program));

    glslang_program_SPIRV_get(program, SPIRV.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
    {
        LOG_DEBUG(spirv_messages);
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return SPIRV.size();
}

static size_t compileShaderFile(const std::string& filePath, ShaderBinary& SPIRV)
{
    std::string shaderSource = readShaderFile(filePath);
    if (!shaderSource.empty())
    {
        return compileShader(shaderSource, glslangShaderStageFromFileName(filePath), SPIRV);
    }

    return 0;
}

static void saveSPIRVBinaryFile(const std::string& filePath, ShaderBinary& binary, size_t size)
{
    std::ofstream outFile(filePath, std::ios::binary | std::ios::out);

    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open file to save shader binary: ", filePath, "\n");
        return;
    }

    outFile.write((const char*)binary.data(), size * sizeof(u32));
    outFile.close();
}

static size_t readSPIRVBinaryFile(const std::string& filePath, ShaderBinary& buffer)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to open SPIRV file: ", filePath);
        return false;
    }

    size_t fileSize = (size_t)file.tellg();

    buffer.resize(fileSize / sizeof(u32));

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    return fileSize;
}

bool VKRenderDevice::createShader(const std::string& filePath, ShaderModule& shader)
{
    fs::path binaryPath(filePath + ".spv");
    ShaderBinary SPIRV;
    size_t size;
    // bool binaryExists = fs::exists(binaryPath);

    // if (binaryExists)
    // {
    //     size = readSPIRVBinaryFile(binaryPath.string(), SPIRV);
    // }
    // else
    // {
    size = compileShaderFile(filePath, SPIRV);
    // }

    if (size == 0)
    {
        return false;
    }

    shader.SPIRV = SPIRV;
    shader.size = size;
    shader.binaryPath = binaryPath.string();

    const VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader.SPIRV.size() * sizeof(u32),
        .pCode = shader.SPIRV.data(),
    };

    vkCreateShaderModule(mDevice.getLogical(), &createInfo, nullptr, &shader.shaderModule);

    // if (!binaryExists)
    // {
    saveSPIRVBinaryFile(binaryPath.string(), SPIRV, size);
    // }

    return true;
}

void VKRenderDevice::destroyShader(ShaderModule& shader)
{
    vkDestroyShaderModule(mDevice.getLogical(), shader.shaderModule, nullptr);

    shader.SPIRV.clear();
    shader.binaryPath = "";
    shader.size = 0;
    shader.shaderModule = nullptr;
}

/*
 * Graphics pipeline functions
 */
bool VKRenderDevice::createPipelineLayout(const VkDescriptorSetLayout& descriptorLayout, VkPipelineLayout& pipelineLayout)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    VK_CHECK(vkCreatePipelineLayout(mDevice.getLogical(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

    return true;
}

bool VKRenderDevice::createPipelineLayoutWithConstants(const VkDescriptorSetLayout& descriptorLayout, VkPipelineLayout& pipelineLayout, u32 vertexConstSize, u32 fragConstSize)
{
    const VkPushConstantRange ranges[] = {
        {
            VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
            0,                          // offset
            vertexConstSize             // size
        },

        {
            VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
            vertexConstSize,              // offset
            fragConstSize                 // size
        },
    };

    u32 constSize = (vertexConstSize > 0) + (fragConstSize > 0);

    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorLayout,
        .pushConstantRangeCount = constSize,
        .pPushConstantRanges = (constSize == 0) ? nullptr : (vertexConstSize > 0 ? ranges : &ranges[1])};

    VK_CHECK(vkCreatePipelineLayout(mDevice.getLogical(), &pipelineLayoutInfo, nullptr, &pipelineLayout));

    return true;
}

bool VKRenderDevice::createColorDepthRenderPass(const RenderPassCreateInfo& createInfo, bool useDepth, VkRenderPass& renderPass, VkFormat colorFormat)
{
    const bool offscreenInternal = createInfo.flags & eRenderPassBit::eRenderPassBitOffscreenInternal;
    const bool first = createInfo.flags & eRenderPassBit::eRenderPassBitFirst;
    const bool last = createInfo.flags & eRenderPassBit::eRenderPassBitLast;

    VkAttachmentDescription colorAttachment = {
        .flags = 0,
        .format = colorFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = offscreenInternal ? VK_ATTACHMENT_LOAD_OP_LOAD : (createInfo.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInternal ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        .finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment = {
        .flags = 0,
        .format = useDepth ? findDepthFormat() : /* this is a dummy format, this attachemnt wont be used */ VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = offscreenInternal ? VK_ATTACHMENT_LOAD_OP_LOAD : (createInfo.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = createInfo.clearDepth ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInternal ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    if (createInfo.flags & eRenderPassBit::eRenderPassBitOffscreen)
    {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    std::vector<VkSubpassDependency> subpassDependencies = {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        },
    };

    if (createInfo.flags & eRenderPassBit::eRenderPassBitOffscreen)
    {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // for layout transitions
        subpassDependencies.resize(2);

        subpassDependencies[0] = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        };

        subpassDependencies[1] = {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        };
    }

    // single subpass using only color and depth buffers
    const VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    const VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = (u32)(useDepth ? 2 : 1),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = (u32)subpassDependencies.size(),
        .pDependencies = subpassDependencies.data(),
    };

    VK_CHECK(vkCreateRenderPass(mDevice.getLogical(), &renderPassInfo, nullptr, &renderPass));

    return true;
}

bool VKRenderDevice::createGraphicsPipeline(u32 width, u32 height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
                                            const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, VkPrimitiveTopology topology, VkPipeline& pipeline,
                                            bool useDepth, bool useBlending, bool dynamicScissorState)
{
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {width, height},
    };

    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    /*
     * Multisampling, always disabled for now
     */
    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}};

    // depth/stencil state
    const VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
        .depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    // dynamic state
    VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;
    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = 1,
        .pDynamicStates = &dynamicStateElt,
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (u32)shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = useDepth ? &depthStencil : nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
        .layout = pipelineLayout,
        .renderPass = renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VK_CHECK(vkCreateGraphicsPipelines(mDevice.getLogical(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    return true;
}

static VkShaderStageFlagBits glslangShaderStageToVulkan(glslang_stage_t sh)
{
    switch (sh)
    {
    case GLSLANG_STAGE_VERTEX:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case GLSLANG_STAGE_FRAGMENT:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case GLSLANG_STAGE_GEOMETRY:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case GLSLANG_STAGE_TESSCONTROL:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case GLSLANG_STAGE_TESSEVALUATION:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case GLSLANG_STAGE_COMPUTE:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    return VK_SHADER_STAGE_VERTEX_BIT;
}

bool VKRenderDevice::createGraphicsPipeline(u32 width, u32 height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
                                            const std::vector<std::string>& shaderFilePaths, VkPrimitiveTopology topology, VkPipeline& pipeline,
                                            bool useDepth, bool useBlending, bool dynamicScissorState)
{
    std::vector<ShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    shaderModules.resize(shaderFilePaths.size());
    shaderStages.resize(shaderFilePaths.size());

    for (int i = 0; i < shaderFilePaths.size(); i++)
    {
        const auto& shaderFile = shaderFilePaths[i];

        createShader(shaderFile, shaderModules[i]);

        VkShaderStageFlagBits stage = glslangShaderStageToVulkan(glslangShaderStageFromFileName(shaderFile));

        shaderStages[i] = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = stage,
            .module = shaderModules[i].shaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };
    }

    createGraphicsPipeline(width, height, renderPass, pipelineLayout, shaderStages, topology, pipeline, useDepth, useBlending, dynamicScissorState);

    for (auto& shader : shaderModules)
    {
        destroyShader(shader);
    }

    return true;
}

bool VKRenderDevice::createColorDepthSwapchainFramebuffers(VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers)
{
    const int imageCount = mSwapchain.getImageCount();

    swapchainFramebuffers.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++)
    {
        std::array<VkImageView, 2> attachments = {
            mSwapchain.getImageView(i),
            depthImageView};

        const VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
            .pAttachments = attachments.data(),
            .width = mSwapchain.getExtent().width,
            .height = mSwapchain.getExtent().height,
            .layers = 1};

        VK_CHECK(vkCreateFramebuffer(mDevice.getLogical(), &framebufferInfo, nullptr, &swapchainFramebuffers[i]));
    }

    return true;
}

void VKRenderDevice::destroyRenderPass(VkRenderPass renderPass)
{
    vkDestroyRenderPass(mDevice.getLogical(), renderPass, nullptr);
}

void VKRenderDevice::destroyPipeline(VkPipeline pipeline)
{
    vkDestroyPipeline(mDevice.getLogical(), pipeline, nullptr);
}

void VKRenderDevice::destroyPipelineLayout(VkPipelineLayout pipelineLayout)
{
    vkDestroyPipelineLayout(mDevice.getLogical(), pipelineLayout, nullptr);
}

void VKRenderDevice::destroyFramebuffer(VkFramebuffer frameBuffer)
{
    vkDestroyFramebuffer(mDevice.getLogical(), frameBuffer, nullptr);
}

} // namespace Suoh
