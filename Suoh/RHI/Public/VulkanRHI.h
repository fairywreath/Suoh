#pragma once

#include "RHI.h"

#include <vector>

//#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
//#include "Vulkan/Headers/vulkan.hpp"

namespace SuohRHI
{

namespace Vulkan
{

class IDevice : public SuohRHI::IDevice
{
    /*
     * Vulkan specific methods.
     */
};

using DeviceHandle = RefCountPtr<IDevice>;

struct DeviceDesc
{
    // std::vector<const char*> layers;
    // std::vector<const char*> instanceExtensions;
    std::vector<const char*> deviceExtensions;

    u32 framebufferWidth;
    u32 framebufferHeight;

    // XXX: workaround for now since we do not have an "instance/device manager" object
    // VkSurfaceKHR surface;
    void* glfwWindowPtr;
};

[[nodiscard]] DeviceHandle CreateDevice(const DeviceDesc& desc);

} // namespace Vulkan

} // namespace SuohRHI
