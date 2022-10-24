#pragma once

#include "VulkanCommon.h"

namespace RenderCore
{

namespace Vulkan
{

/*
 * Buffers.
 */
vk::BufferUsageFlags ToVkBufferUsageFlags(BufferUsage usage);

u32 BytesPerTexFormat(vk::Format format);

bool HasStencilComponent(vk::Format format);

glslang_stage_t ToGlslangShaderStageFromFileName(const std::string& fileName);

} // namespace Vulkan

} // namespace RenderCore