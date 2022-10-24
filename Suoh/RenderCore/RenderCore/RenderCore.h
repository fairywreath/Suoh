#pragma once

#include <CoreTypes.h>

#include <memory>
#include <vector>

#include "RenderStates.h"

namespace RenderCore
{

struct DeviceDesc
{
    std::vector<const char*> deviceExtensions;

    u32 framebufferWidth;
    u32 framebufferHeight;

    void* glfwWindowPtr;
};

namespace Vulkan
{

class VulkanDevice;
;
} // namespace Vulkan

std::unique_ptr<Vulkan::VulkanDevice> CreateVulkanDevice(const DeviceDesc& desc);

} // namespace RenderCore
