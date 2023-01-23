#pragma once

#include "Graphics/Vulkan/VulkanResources.h"

enum class ShaderLanguage
{
    GLSL,
    HLSL,
};

namespace SPIRV
{

[[nodiscard]] std::string ReadShaderFile(const std::string& fileName);

/*
 * Compile GLSL shader by running glslang through the command line.
 */
void CompileShaderFileCommandLine(const std::string& dstFile, const std::string& srcFile,
                                  vk::ShaderStageFlagBits shaderStage);

// [[nodiscard]] std::vector<u32> CompileShaderSource(const std::string& shaderSource,
// vk::ShaderStageFlagBits shaderStage);

[[nodiscard]] std::vector<u32> CompileShaderFile(const std::string& srcFile,
                                                 vk::ShaderStageFlagBits shaderStage);

size_t ReadShaderBinaryFile(const std::string& fileName, std::vector<u32>& buffer);

} // namespace SPIRV
