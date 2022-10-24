#include "VulkanShader.h"

#include "VulkanUtils.h"

#include <filesystem>
#include <fstream>

// glslang::TBuiltInResource....
#include <StandAlone/ResourceLimits.h>

namespace fs = std::filesystem;

namespace RenderCore
{

namespace Vulkan
{

Shader::~Shader()
{
    if (shaderModule)
    {
        m_Context.device.destroyShaderModule(shaderModule);
    }
}

std::string ReadShaderFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    fs::path path(filePath);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to read shader file: ", fs::absolute(path));
        return std::string();
    }

    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    while (code.find("#include ") != code.npos)
    {
        const auto pos = code.find("#include ");
        const auto p1 = code.find('<', pos);
        const auto p2 = code.find('>', pos);
        if (p1 == code.npos || p2 == code.npos || p2 <= p1)
        {
            LOG_ERROR("Failed loading shader program: ", code);
            file.close();
            return std::string();
        }
        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);

        // XXX: read from relative address of current shader file.
        const std::string include = ReadShaderFile(name.c_str());

        code.replace(pos, p2 - pos + 1, include.c_str());
    }

    file.close();
    return code;
}

size_t CompileShader(const std::string& shaderSource, glslang_stage_t stage, ShaderBinary& SPIRV)
{
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_1,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_3,
        .code = shaderSource.c_str(),
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = reinterpret_cast<const glslang_resource_t*>(&glslang::DefaultTBuiltInResource),
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input))
    {
        LOG_ERROR("GLSL preprocessing failed");
        LOG_ERROR(glslang_shader_get_info_log(shader));
        LOG_ERROR(glslang_shader_get_info_debug_log(shader));
        LOG_ERROR("Source: ");

        LogShaderSource(shaderSource);
        return 0;
    }

    if (!glslang_shader_parse(shader, &input))
    {
        LOG_ERROR("GLSL parsing failed");
        LOG_ERROR("\n", glslang_shader_get_info_log(shader),
                  glslang_shader_get_info_debug_log(shader));

        // LOG_ERROR(glslang_shader_get_preprocessed_code(shader));
        LogShaderSource(shaderSource);
        return 0;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    int msgs = GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT;

    if (!glslang_program_link(program, msgs))
    {
        LOG_ERROR("GLSL linking failed");
        LOG_ERROR(glslang_program_get_info_log(program));
        LOG_ERROR(glslang_program_get_info_debug_log(program));
        return 0;
    }

    glslang_program_SPIRV_generate(program, stage);

    SPIRV.resize(glslang_program_SPIRV_get_size(program));

    glslang_program_SPIRV_get(program, SPIRV.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
    {
        LOG_DEBUG(spirv_messages);
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return SPIRV.size();
}

size_t CompileShaderFile(const std::string& filePath, ShaderBinary& binary)
{
    std::string shaderSource = ReadShaderFile(filePath);
    if (!shaderSource.empty())
    {
        return CompileShader(shaderSource, ToGlslangShaderStageFromFileName(filePath), binary);
    }

    return 0;
}

void SaveShaderBinaryFile(const std::string& filePath, ShaderBinary& binary, size_t size)
{
    std::ofstream outFile(filePath, std::ios::binary | std::ios::out);

    if (!outFile.is_open())
    {
        LOG_ERROR("Failed to open file to save shader binary: ", filePath, "\n");
        return;
    }

    outFile.write((const char*)binary.data(), size * sizeof(u32));
    outFile.close();
}

size_t ReadShaderBinaryFile(const std::string& filePath, ShaderBinary& buffer)
{
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to open SPIRV file: ", filePath);
        return false;
    }

    size_t fileSize = (size_t)file.tellg();

    buffer.resize(fileSize / sizeof(u32));

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    return fileSize;
}

void LogShaderSource(const std::string& source)
{
    if (source.empty())
    {
        return;
    }

    int lineNum = 1;
    std::string line = std::to_string(lineNum) + " ";

    for (const auto& c : source)
    {
        if (c == '\n')
        {
            LOG_DEBUG(line);
            line = std::to_string(++lineNum) + " ";
        }
        else if (c == '\r')
        {
        }
        else
        {
            line += c;
        }
    }

    if (!line.empty())
    {
        LOG_DEBUG(line);
    }
}

} // namespace Vulkan

} // namespace RenderCore
