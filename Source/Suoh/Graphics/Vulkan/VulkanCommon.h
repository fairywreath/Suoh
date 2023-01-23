#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <Core/Containers.h>
#include <Core/Logger.h>
#include <Core/Types.h>

#include "Graphics/GPUStates.h"

#include "GPUConstants.h"
#include "GPUResource.h"

#include "VulkanUtils.h"

#define VULKAN_DEBUG

#define VK_CHECK(res)                                                                                                  \
    if ((res) != vk::Result::eSuccess)                                                                                 \
    {                                                                                                                  \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                     \
        abort();                                                                                                       \
    }
#define VK_CHECK_RETURN(res)                                                                                           \
    if ((res) != vk::Result::eSuccess)                                                                                 \
    {                                                                                                                  \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                     \
        return;                                                                                                        \
    }
#define VK_CHECK_RETURN_RESULT(res)                                                                                    \
    if ((res) != vk::Result::eSuccess)                                                                                 \
    {                                                                                                                  \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                     \
        return res;                                                                                                    \
    }
#define VK_CHECK_RETURN_NULL(res)                                                                                      \
    if ((res) != vk::Result::eSuccess)                                                                                 \
    {                                                                                                                  \
        LOG_ERROR("Detected Vulkan error: ", res);                                                                     \
        return nullptr;                                                                                                \
    }

#define VK_ASSERT_OK(res) assert((res) == vk::Result::eSuccess)

#define VERIFYVULKANRESULT(res)

static constexpr std::array S_RequestedVulkanInstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(_WIN32)
    "VK_KHR_win32_surface",
#endif

// XXX: Provide surface instance extensions for other platforms.

#if defined(VULKAN_DEBUG)
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

static constexpr std::array S_ReqeustedVulkanInstanceLayers = {
#if defined(VULKAN_DEBUG)
    "VK_LAYER_KHRONOS_validation",
#else
    "",
#endif
};

template <typename T>
auto VulkanHandleToU64(T handle)
{
    return u64(static_cast<typename T::CType>(handle));
}

inline bool IsDepthStencil(vk::Format value)
{
    return value == vk::Format::eD16UnormS8Uint || value == vk::Format::eD24UnormS8Uint
           || value == vk::Format::eD32SfloatS8Uint;
}
inline bool IsDepthOnly(vk::Format value)
{
    return value >= vk::Format::eD16Unorm && value < vk::Format::eD32Sfloat;
}
inline bool IsStencilOnly(vk::Format value)
{
    return value == vk::Format::eS8Uint;
}
inline bool HasDepth(vk::Format value)
{
    return (value >= vk::Format::eD16Unorm && value < vk::Format::eS8Uint)
           || (value >= vk::Format::eD16UnormS8Uint && value <= vk::Format::eD32SfloatS8Uint);
}
inline bool HasStencil(vk::Format value)
{
    return value >= vk::Format::eS8Uint && value <= vk::Format::eD32SfloatS8Uint;
}
inline bool HasDepthOrStencil(vk::Format value)
{
    return value >= vk::Format::eD16Unorm && value <= vk::Format::eD32SfloatS8Uint;
}
