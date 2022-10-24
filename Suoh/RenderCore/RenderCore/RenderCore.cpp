#include "RenderCore.h"

#include "Vulkan/VulkanDevice.h"

namespace RenderCore
{

std::unique_ptr<Vulkan::VulkanDevice> CreateVulkanDevice(const DeviceDesc& desc)
{
    return std::make_unique<Vulkan::VulkanDevice>(desc);
}

} // namespace RenderCore