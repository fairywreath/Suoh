#include "VulkanRHI.h"

#include "VulkanCommon.h"
#include "VulkanDevice.h"

namespace SuohRHI
{

namespace Vulkan
{

[[nodiscard]] DeviceHandle CreateDevice(const DeviceDesc& desc)
{
    auto device = new VulkanDevice(desc);
    return DeviceHandle::Create(device);
}

} // namespace Vulkan

} // namespace SuohRHI
