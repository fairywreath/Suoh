
#include "VulkanDevice.h"

#include "Graphics/SPIRV/ShaderCompiler.h"

#include <Core/Logger.h>

ShaderState::ShaderState(VulkanDevice* device, const ShaderStateDesc& desc) : m_Device(std::move(device))
{
    const u32 stagesCount = desc.shaderStages.size();
    if (stagesCount == 0)
    {
        LOG_ERROR("ShaderStateDesc does not contain any stages!");
        return;
    }

    m_PipelineType = PipelineType::Graphics;
    m_Name = desc.name;

    m_VulkanShaderStages.resize(desc.shaderStages.size());

    u32 compiledShaders = 0;
    u32 brokenStage = u32(-1);

    for (compiledShaders = 0; compiledShaders < stagesCount; compiledShaders++)
    {
        auto& shaderStage = desc.shaderStages[compiledShaders];
        if (shaderStage.shaderStageFlags == vk::ShaderStageFlagBits::eCompute)
        {
            // XXX TODO: Check only compute shaders exist for this pipeline.
            m_PipelineType = PipelineType::Compute;
        }

        vk::ShaderModuleCreateInfo shaderModuleInfo{};
        bool compiled = false;
        std::vector<u32> shaderBinary;

        if (shaderStage.spirvInput)
        {
            if (shaderStage.readFromFile)
            {
                assert(std::holds_alternative<std::string>(shaderStage.data));
                auto binaryFileName = get<std::string>(shaderStage.data);
                SPIRV::ReadShaderBinaryFile(binaryFileName, shaderBinary);
            }
            else
            {
                assert(std::holds_alternative<std::vector<u32>>(shaderStage.data));
                shaderBinary = get<std::vector<u32>>(shaderStage.data);
            }
        }
        else
        {
            if (shaderStage.readFromFile)
            {
                assert(std::holds_alternative<std::string>(shaderStage.data));
                auto shaderFileName = get<std::string>(shaderStage.data);
                shaderBinary = SPIRV::CompileShaderFile(shaderFileName, shaderStage.shaderStageFlags);
            }
            else
            {
                assert(std::holds_alternative<std::vector<u32>>(shaderStage.data));
                auto shaderSource = get<std::string>(shaderStage.data);

                // XXX TODO: Compile from source code directly.
                assert(false, "Compiling shader directly from source string is not yet implemented!");
            }
        }

        shaderModuleInfo.setCode(shaderBinary);

        auto& shaderStageInfo = m_VulkanShaderStages[compiledShaders];
        shaderStageInfo.setStage(shaderStage.shaderStageFlags).setPName("main");

        if (m_Device->GetVulkanDevice().createShaderModule(&shaderModuleInfo, m_Device->GetVulkanAllocationCallbacks(),
                                                           &shaderStageInfo.module)
            != vk::Result::eSuccess)
        {
            brokenStage = compiledShaders;
            break;
        }

        // XXX TODO:
        // SPIRVParser::ParseSPIRVBinary(shaderBinary, shaderState->parseResult);

        m_Device->SetVulkanResourceName(vk::ObjectType::eShaderModule, shaderStageInfo.module, desc.name);
    }

    if (compiledShaders != stagesCount)
    {
        // XXX: Nicer error logs.
        LOG_ERROR("Failed creating shader state, error compiling shader at index: ", brokenStage);

        assert(!m_VulkanShaderStages[compiledShaders].module);

        // Resize shader stages to succesfully compiled shaders only.
        m_VulkanShaderStages.resize(compiledShaders - 1);

        // Destroy successfully created shader modules.
        DestroyShaderModules();
    }
}

void ShaderState::DestroyShaderModules()
{
    for (auto& stage : m_VulkanShaderStages)
    {
        assert(stage.module);
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::ShaderModule, stage.module);
    }

    m_VulkanShaderStages.clear();
}

ShaderState::~ShaderState()
{
    DestroyShaderModules();
}

ShaderStateRef VulkanDevice::CreateShaderState(const ShaderStateDesc& desc)
{
    auto shaderRef = ShaderStateRef::Create(new ShaderState(this, desc));

    if (shaderRef->GetNumStages() == 0)
    {
        LOG_ERROR("CreateShaderState: No vulkan shader stages present!");
        shaderRef = nullptr;
    }

    return shaderRef;
}