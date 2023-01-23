#include "ShaderCompiler.h"

#include <filesystem>
#include <fstream>

#include <Core/Logger.h>
#include <Core/SystemCommand.h>

namespace fs = std::filesystem;

namespace SPIRV
{

static constexpr auto ShadersDirectory = "Shaders/";

std::string ReadShaderFile(const std::string& fileName)
{
    std::ifstream file(fileName);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to read shader file: ", fs::absolute(fileName));
        return "";
    }

    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    while (code.find("#include ") != code.npos)
    {
        const auto pos = code.find("#include ");

        const auto p1 = code.find('"', pos);

        // Skip over length of '#include ' + 1 for first quotation mark.
        const auto p2 = code.find('"', pos + 10);

        if (p1 == code.npos || p2 == code.npos || p2 <= p1)
        {
            LOG_ERROR("Failed loading shader file: ", p1, " ", p2, " ", code);
            return "";
        }

        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);

        // XXX: Use non-recursive implementation.
        const std::string include = ReadShaderFile(ShadersDirectory + name);

        code.replace(pos, p2 - pos + 1, include.c_str());
    }

    return code;
}

void CompileShaderFileCommandLine(const std::string& dstFile, const std::string& srcFile,
                                  vk::ShaderStageFlagBits shaderStage)
{
    std::string stageDefines = ToShaderStageDefine(shaderStage);
    std::string command = std::format("glslangValidator.exe {} -V --target-env vulkan1.1 -o {} -S {} --D {}", srcFile,
                                      dstFile, ToShaderCompilerExtension(shaderStage), stageDefines);

    auto res = SystemCommand::Execute(command);

    LOG_INFO("Shader compilation output: ");
    LOG_INFO(res.output);

    // XXX TODO: Perform SPIRV optimizations.
}

[[nodiscard]] std::vector<u32> CompileShaderFile(const std::string& fileName, vk::ShaderStageFlagBits shaderStage)
{
    auto code = ReadShaderFile(fileName);

    const std::string shaderCodeFileName = "tempShaderOut.txt";
    std::ofstream tempOutFile(shaderCodeFileName);
    if (!tempOutFile.good())
    {
        LOG_ERROR("Failed to open file for temporary shader writing: ", fs::absolute(shaderCodeFileName));
        return {};
    }

    tempOutFile << code;
    tempOutFile.close();

    const std::string shaderBinaryFileName = fileName + ".spv";

    CompileShaderFileCommandLine(shaderBinaryFileName, shaderCodeFileName, shaderStage);

    std::vector<u32> binaryResult;
    auto resultSize = ReadShaderBinaryFile(shaderBinaryFileName, binaryResult);

    fs::remove(shaderCodeFileName);
    // fs::remove(shaderBinaryFileName);

    return binaryResult;
}

size_t ReadShaderBinaryFile(const std::string& fileName, std::vector<u32>& buffer)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to open SPIR-V file: ", fs::absolute(fileName));
        return false;
    }

    size_t fileSize = static_cast<size_t>(file.tellg());

    buffer.resize(fileSize / sizeof(u32));

    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    return fileSize;
}

} // namespace SPIRV