#pragma once

#include "VulkanCommon.h"

#include <string>

namespace RenderLib
{

namespace Vulkan
{

enum class ShaderType : u8
{
    VERTEX,
    FRAGMENT,
    GEOMETRY,
    COMPUTE,
};

using ShaderBinary = std::vector<u32>;

struct ShaderDesc
{
    ShaderBinary binary;
    size_t size{0};

    std::string fileName;

    // If data reads to be read from file.
    bool readFile{true};

    // If file path is to a shader src code.
    bool needsCompilation{true};

    ShaderType type{ShaderType::VERTEX};
};

class Shader : public RefCountResource<IResource>
{
public:
    explicit Shader(const VulkanContext& context) : m_Context(context)
    {
    }
    ~Shader();

    vk::ShaderModule shaderModule;

    ShaderDesc desc;

private:
    const VulkanContext& m_Context;
};

using ShaderHandle = RefCountPtr<Shader>;

// Shader helper functions.
std::string ReadShaderFile(const std::string& filePath);

size_t CompileShader(const std::string& shaderSource, glslang_stage_t stage, ShaderBinary& SPIRV);
size_t CompileShaderFile(const std::string& filePath, ShaderBinary& binary);

void SaveShaderBinaryFile(const std::string& filePath, ShaderBinary& binary, size_t size);
size_t ReadShaderBinaryFile(const std::string& filePath, ShaderBinary& buffer);

void LogShaderSource(const std::string& source);

} // namespace Vulkan

} // namespace RenderLib
