#pragma once

#include "CoreTypes.h"

#include "VulkanRHI.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Vulkan/Headers/vulkan.hpp"
//#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

#include <Logger.h>

#include <string>

// #define _DEBUG

namespace SuohRHI
{

template <typename T, typename U> T checked_cast(U u)
{
    static_assert(!std::is_same<T, U>::value, "Redundant checked_cast");
#ifdef _DEBUG
    if (!u)
        return nullptr;
    T t = dynamic_cast<T>(u);
    if (!t)
        assert(!"Invalid type cast!");
    return t;
#else
    return static_cast<T>(u);
#endif
}

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

#define VK_CHECK_RETURN(res)                                                                                                               \
    if ((res) != vk::Result::eSuccess)                                                                                                     \
    {                                                                                                                                      \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                                         \
        return;                                                                                                                            \
    }
#define VK_CHECK_RETURN_RESULT(res)                                                                                                        \
    if ((res) != vk::Result::eSuccess)                                                                                                     \
    {                                                                                                                                      \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                                         \
        return res;                                                                                                                        \
    }
#define VK_CHECK_RETURN_NULL(res)                                                                                                          \
    if ((res) != vk::Result::eSuccess)                                                                                                     \
    {                                                                                                                                      \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                                         \
        return nullptr;                                                                                                                    \
    }

#if VK_DEBUG
#define VK_ASSERT_OK(res) assert((res) == vk::Result::eSuccess)
#else
#define VK_ASSERT_OK(res)                                                                                                                  \
    do                                                                                                                                     \
    {                                                                                                                                      \
        (void)(res);                                                                                                                       \
    } while (0)
#endif

const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#if VK_DEBUG
static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

struct VulkanContext
{
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;

    vk::SurfaceKHR surface;

    vk::PipelineCache pipelineCache;

    VmaAllocator allocator;
};

struct VulkanResourceStateMapping
{
    ResourceStates rhiState;
    vk::PipelineStageFlags stageFlags;
    vk::AccessFlags accessMask;
    vk::ImageLayout imageLayout;
};

} // namespace Vulkan

} // namespace SuohRHI

namespace std
{

template <> struct hash<pair<vk::PipelineStageFlags, vk::PipelineStageFlags>>
{
    size_t operator()(pair<vk::PipelineStageFlags, vk::PipelineStageFlags> const& s) const noexcept
    {
        return (hash<u32>()(u32(s.first)) ^ (hash<u32>()(u32(s.second)) << 16));
    }
};

} // namespace std