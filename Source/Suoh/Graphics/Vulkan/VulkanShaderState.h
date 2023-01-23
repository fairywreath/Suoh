#pragma once

#include "VulkanDescriptorSets.h"

using SPIRVBinary = std::vector<u32>;
using SPIRVBinaryView = std::span<u32>;

struct ShaderStage
{
    // Binary data is external and not owned by the ShaderStage struct.
    std::variant<SPIRVBinaryView, std::string> data{nullptr};

    vk::ShaderStageFlagBits shaderStageFlags{vk::ShaderStageFlagBits::eAll};

    // If true, input data is in spirv binary format. If false, input data is a shader source string.
    bool spirvInput{true};

    // If true, the data variable contains the file name that contains the data to be read. If false, data is the data
    // itself.
    bool readFromFile{false};

    ShaderStage() = default;
    ShaderStage(SPIRVBinaryView binary, vk::ShaderStageFlagBits flags)
        : data(binary), shaderStageFlags(flags), spirvInput(true), readFromFile(false)
    {
    }
    ShaderStage(const std::string& stringData, vk::ShaderStageFlagBits flags, bool isSpirv, bool readFile)
        : data(stringData), shaderStageFlags(flags), spirvInput(isSpirv), readFromFile(readFile)
    {
    }

    static ShaderStage FromSPIRVBinary(SPIRVBinaryView binary, vk::ShaderStageFlagBits flags)
    {
        return ShaderStage(binary, flags);
    }
    static ShaderStage FromSPIRVFile(const std::string& fileName, vk::ShaderStageFlagBits flags)
    {
        return ShaderStage(fileName, flags, true, true);
    }
    static ShaderStage FromShaderSourceString(const std::string& fileName, vk::ShaderStageFlagBits flags)
    {
        return ShaderStage(fileName, flags, false, false);
    }
    static ShaderStage FromShaderSourceFile(const std::string& fileName, vk::ShaderStageFlagBits flags)
    {
        return ShaderStage(fileName, flags, false, true);
    }
};

using ShaderStageArray = static_vector<ShaderStage, GPUConstants::MaxShaderStagesPerState>;

struct ShaderStateDesc
{
    ShaderStageArray shaderStages{};
    std::string name;

    ShaderStateDesc& Reset()
    {
        shaderStages = {};
        name = "";
        return *this;
    };
    ShaderStateDesc& SetName(const std::string& _name)
    {
        name = _name;
        return *this;
    }
    ShaderStateDesc& AddStage(const ShaderStage& stage)
    {
        shaderStages.push_back(stage);
        return *this;
    }
};

struct SPIRVReflectionResult
{
    std::vector<DescriptorSetLayoutDesc> descriptorSetLayouts;
    ComputeLocalSize computeLocalSize;
};

class ShaderState : public GPUResource
{
public:
    ShaderState(VulkanDevice* device, const ShaderStateDesc& desc);
    virtual ~ShaderState();

    const ShaderStateDesc& GetDesc() const
    {
        return m_Desc;
    }

    const std::vector<vk::PipelineShaderStageCreateInfo>& GetVulkanShaderStages() const
    {
        return m_VulkanShaderStages;
    }

    u32 GetNumStages() const
    {
        return m_VulkanShaderStages.size();
    }

private:
    void DestroyShaderModules();

private:
    VulkanDevice* m_Device;
    ShaderStateDesc m_Desc;

    std::vector<vk::PipelineShaderStageCreateInfo> m_VulkanShaderStages;
    PipelineType m_PipelineType;
    std::string m_Name;

    SPIRVReflectionResult m_ReflectionResult;
};