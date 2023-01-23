#pragma once

#include <Core/Types.h>

#include <vulkan/vulkan.hpp>

void VerifyVulkanResult(vk::Result result, const char* vkFunction, const char* fileName, u32 line);