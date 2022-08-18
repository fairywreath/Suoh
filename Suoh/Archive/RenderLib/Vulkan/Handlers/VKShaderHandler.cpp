#include "VKShaderHandler.h"
#include "../VKRenderDevice.h"

#include <StandAlone/ResourceLimits.h>
#include <glslang/Include/glslang_c_interface.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <queue>

#include <Asserts.h>
#include <Logger.h>

namespace fs = std::filesystem;

namespace Suou
{

using ShaderBinary = std::vector<u32>;

static std::string readShaderFile(const std::string& filePath)
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
        const std::string include = readShaderFile(name.c_str());
        code.replace(pos, p2 - pos + 1, include.c_str());
    }

    file.close();
    return code;
}

static void printShaderSource(const std::string& source)
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

static glslang_stage_t glslangShaderStageFromFileName(const std::string& filePath)
{
    if (filePath.ends_with(".vert"))
        return GLSLANG_STAGE_VERTEX;

    if (filePath.ends_with(".frag"))
        return GLSLANG_STAGE_FRAGMENT;

    if (filePath.ends_with(".geom"))
        return GLSLANG_STAGE_GEOMETRY;

    if (filePath.ends_with(".comp"))
        return GLSLANG_STAGE_COMPUTE;

    if (filePath.ends_with(".tesc"))
        return GLSLANG_STAGE_TESSCONTROL;

    if (filePath.ends_with(".tese"))
        return GLSLANG_STAGE_TESSEVALUATION;

    return GLSLANG_STAGE_VERTEX;
}

static size_t compileShader(const std::string& shaderSource, glslang_stage_t stage, ShaderBinary& SPIRV)
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
        .resource = (const glslang_resource_t*)&glslang::DefaultTBuiltInResource,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    // XXX: need to redirect these logs to the internal logging library

    if (!glslang_shader_preprocess(shader, &input))
    {
        LOG_ERROR("GLSL preprocessing failed");
        LOG_ERROR(glslang_shader_get_info_log(shader));
        LOG_ERROR(glslang_shader_get_info_debug_log(shader));
        LOG_ERROR("Source: ");

        // LOG_ERROR(input.code);
        printShaderSource(shaderSource);
        return 0;
    }

    if (!glslang_shader_parse(shader, &input))
    {
        LOG_ERROR("GLSL parsing failed");
        LOG_ERROR("\n", glslang_shader_get_info_log(shader),
                  glslang_shader_get_info_debug_log(shader));

        // LOG_ERROR(glslang_shader_get_preprocessed_code(shader));
        printShaderSource(shaderSource);
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

static size_t compileShaderFile(const std::string& filePath, ShaderBinary& SPIRV)
{
    std::string shaderSource = readShaderFile(filePath);
    if (!shaderSource.empty())
    {
        return compileShader(shaderSource, glslangShaderStageFromFileName(filePath), SPIRV);
    }

    return 0;
}

static void saveSPIRVBinaryFile(const std::string& filePath, ShaderBinary& binary, size_t size)
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

static size_t readSPIRVBinaryFile(const std::string& filePath, ShaderBinary& buffer)
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

struct ShaderModule
{
    ShaderBinary SPIRV;
    size_t size;
    std::string binaryPath;

    VkShaderModule shaderModule;
};

struct VKShaderHandlerData : IVKShaderHandlerData
{
    std::vector<ShaderModule> shaders;
    std::queue<ShaderHandle> returnedShaderHandles;
};

static constexpr VKShaderHandlerData& toShaderHandlerData(IVKShaderHandlerData* data)
{
    return static_cast<VKShaderHandlerData&>(*data);
}

VKShaderHandler::VKShaderHandler(VKRenderDevice& renderDevice)
    : mRenderDevice(renderDevice),
      mData(std::make_unique<VKShaderHandlerData>())
{
    glslang_initialize_process();
}

VKShaderHandler::~VKShaderHandler()
{
    glslang_finalize_process();
}

ShaderHandle VKShaderHandler::createShader(const ShaderDescription& desc)
{
    fs::path binaryPath(desc.filePath + ".spv");
    ShaderBinary SPIRV;
    size_t size;
    bool binaryExists = fs::exists(binaryPath);

    if (binaryExists)
    {
        size = readSPIRVBinaryFile(binaryPath.string(), SPIRV);
    }
    else
    {
        size = compileShaderFile(desc.filePath, SPIRV);
    }

    if (size == 0)
    {
        return ShaderHandle::Invalid();
    }

    auto& data = toShaderHandlerData(mData.get());
    ShaderHandle handle = acquireNewHandle();
    ShaderModule& shader = data.shaders[toHandleType(handle)];

    SUOU_ASSERT(shader.size == 0);

    shader.SPIRV = SPIRV;
    shader.size = size;
    shader.binaryPath = binaryPath.string();

    const VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader.SPIRV.size() * sizeof(u32),
        .pCode = shader.SPIRV.data(),
    };

    vkCreateShaderModule(mRenderDevice.mDevice.getLogical(), &createInfo, nullptr, &shader.shaderModule);

    if (!binaryExists)
    {
        saveSPIRVBinaryFile(binaryPath.string(), SPIRV, size);
    }

    return handle;
}

void VKShaderHandler::destroyShader(ShaderHandle handle)
{
    HandleType handleValue = toHandleType(handle);
    auto& data = toShaderHandlerData(mData.get());

    SUOU_ASSERT(handleValue < data.shaders.size());

    ShaderModule& shader = data.shaders[handleValue];

    vkDestroyShaderModule(mRenderDevice.mDevice.getLogical(), shader.shaderModule, nullptr);

    shader.SPIRV.clear();
    shader.binaryPath = "";
    shader.size = 0;
    shader.shaderModule = nullptr;

    data.returnedShaderHandles.push(handle);
}

ShaderHandle VKShaderHandler::acquireNewHandle()
{
    ShaderHandle handle;
    auto& data = toShaderHandlerData(mData.get());

    if (!data.returnedShaderHandles.empty())
    {
        handle = data.returnedShaderHandles.front();
        data.returnedShaderHandles.pop();
    }
    else
    {
        handle = ShaderHandle(static_cast<HandleType>(data.shaders.size()));
        data.shaders.emplace_back();
        data.shaders.back().size = 0;
    }

    return handle;
}

} // namespace Suou
