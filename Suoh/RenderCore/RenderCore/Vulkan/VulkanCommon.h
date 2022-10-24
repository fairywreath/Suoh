#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Headers/vulkan.hpp"

#include <vk_mem_alloc.h>

#include <glslang/Include/glslang_c_interface.h>

#include <CoreTypes.h>
#include <Logger.h>

#include <string>

#include "RenderCore/Common/RenderResources.h"
#include "RenderCore/RenderCore.h"

namespace RenderCore
{

namespace Vulkan
{

#define VK_DEBUG 1

#define VK_CHECK_ABORT(res)                                                                        \
    if ((res) != vk::Result::eSuccess)                                                             \
    {                                                                                              \
        LOG_ERROR("Detected Vulkan error: ", res);                                                 \
        abort();                                                                                   \
    }
#define VK_CHECK_RETURN(res)                                                                       \
    if ((res) != vk::Result::eSuccess)                                                             \
    {                                                                                              \
        LOG_ERROR("Detected Vulkan error: ", res);                                                 \
        return;                                                                                    \
    }
#define VK_CHECK_RETURN_RESULT(res)                                                                \
    if ((res) != vk::Result::eSuccess)                                                             \
    {                                                                                              \
        LOG_ERROR("Detected Vulkan error: ", res);                                                 \
        return res;                                                                                \
    }
#define VK_CHECK_RETURN_NULL(res)                                                                  \
    if ((res) != vk::Result::eSuccess)                                                             \
    {                                                                                              \
        LOG_ERROR("Detected Vulkan error: ", res);                                                 \
        return nullptr;                                                                            \
    }
#define VK_ASSERT_OK(res) assert((res) == vk::Result::eSuccess)

#if VK_DEBUG
static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct VulkanContext
{
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::SurfaceKHR surface;

    VmaAllocator allocator;
};

} // namespace Vulkan

} // namespace RenderCore
