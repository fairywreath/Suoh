#include "RenderLib.h"

#include "Vulkan/VulkanDevice.h"

namespace RenderLib
{

std::unique_ptr<Vulkan::VulkanDevice> CreateVulkanDevice(const DeviceDesc& desc)
{
    return std::make_unique<Vulkan::VulkanDevice>(desc);
}

} // namespace RenderLib