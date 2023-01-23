#pragma once

#include "Graphics/Vulkan/VulkanResources.h"

namespace SPIRV
{

void ParseSPIRVBinary(std::span<u32> binaryData, ParseResult& parseResult);

} // namespace SPIRV
