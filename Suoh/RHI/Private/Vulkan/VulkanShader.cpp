#include "VulkanShader.h"

#include "VulkanUtils.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanShader::~VulkanShader()
{
    if (shaderModule)
    {
        m_Context.device.destroyShaderModule(shaderModule);
        shaderModule = vk::ShaderModule();
    }
}

Object VulkanShader::GetNativeObject(ObjectType objectType)
{
    switch (objectType)
    {
    case ObjectTypes::Vk_ShaderModule:
        return Object(shaderModule);
    default:
        return Object(nullptr);
    }
}

void VulkanShader::GetBytecode(const void** ppBytecode, size_t* pSize) const
{
    // XXX: Save binary info somewhere?.
}

VulkanShaderLibrary::~VulkanShaderLibrary()
{
    if (shaderModule)
    {
        m_Context.device.destroyShaderModule(shaderModule);
        shaderModule = vk::ShaderModule();
    }
}

ShaderHandle VulkanShaderLibrary::GetShader(const char* entryName, ShaderType type)
{
    auto* shader = new VulkanShader(m_Context);
    shader->desc.entryName = entryName;
    shader->desc.shaderType = type;
    shader->shaderModule = shaderModule;
    // Shader specializations..
    // shader->baseShader = this;
    shader->stageFlagBits = ToVkShaderStageFlagBits(type);

    return ShaderHandle::Create(shader);
}

void VulkanShaderLibrary::GetBytecode(const void** ppByteCode, size_t* pSize) const
{
    // XXX: Save binary info somewhere?.
}

u32 VulkanInputLayout::GetNumAttributes() const
{
    return u32(inputDesc.size());
}

const VertexAttributeDesc* VulkanInputLayout::GetAttributeDesc(u32 index) const
{
    if (index < u32(inputDesc.size()))
        return &inputDesc[index];

    return nullptr;
}

} // namespace Vulkan

} // namespace SuohRHI