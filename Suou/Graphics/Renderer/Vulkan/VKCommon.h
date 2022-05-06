#pragma once

#include <vulkan/vulkan.h>

#include <vector>

// XXX: handle debug errors in a better way, use logging???
#include <iostream>

#include <CoreTypes.h>
#include <PlatformDefines.h>

#define VK_DEBUG

namespace Suou
{


#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; 	\
			abort();                                                \
		}                                                           \
	} while (0)


namespace VKCommon
{

const std::vector<const char*> ValidationLayers = 
{
	"VK_LAYER_KHRONOS_validation"
};

#if defined(VK_DEBUG)
static constexpr bool EnableValidationLayers = true;
#else
static constexpr bool EnableValidationLayers = false;
#endif

} // VulkanCommon

} // Suou
