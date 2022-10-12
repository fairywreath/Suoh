#pragma once

#include "CoreTypes.h"

#include "VulkanRHI.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Vulkan/Headers/vulkan.hpp"
//#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

#include <Logger.h>

#include <string>

namespace SuohRHI
{

namespace Vulkan
{

#define VK_DEBUG 1

#define VK_CHECK(x)                                                                                                                        \
    do                                                                                                                                     \
    {                                                                                                                                      \
        vk::Result err = x;                                                                                                                \
        if (err != vk::Result::eSuccess)                                                                                                   \
        {                                                                                                                                  \
            LOG_ERROR("Detected Vulkan error: ", err);                                                                                     \
            abort();                                                                                                                       \
        }                                                                                                                                  \
    } while (0)

const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if defined(VK_DEBUG)
static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

} // namespace Vulkan

} // namespace SuohRHI