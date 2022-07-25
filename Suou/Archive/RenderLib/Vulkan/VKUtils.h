#pragma once

#include <vk_mem_alloc.h>

#include "../RenderStates.h"
#include "VKCommon.h"

namespace Suou
{

namespace VKUtils
{

VkFormat toVkFormat(ImageFormat format);
VkFormat toVkFormat(DepthImageFormat format);
VkSampleCountFlagBits toVkSampleCount(SampleCount count);

VkBufferUsageFlags toVkBufferUsageFlags(u32 usage);
VkImageUsageFlags toVkImageUsageFlags(u8 usage);

VmaMemoryUsage toVmaMemoryUsage(BufferMemoryUsage usage);

} // namespace VKUtils

} // namespace Suou
