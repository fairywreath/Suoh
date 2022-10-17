#pragma once

#include "VulkanCommon.h"

namespace SuohRHI
{

namespace Vulkan
{

/*
 * XXX: Handle shader specializations.
 */

class VulkanShader : public RefCountResource<IShader>
{
public:
    explicit VulkanShader(const VulkanContext& context) : m_Context(context)
    {
        desc.shaderType = ShaderType::NONE;
    }
    ~VulkanShader() override;

    const ShaderDesc& GetDesc() const override
    {
        return desc;
    }
    Object GetNativeObject(ObjectType type) override;

    void GetBytecode(const void** ppBytecode, size_t* pSize) const override;

    ShaderDesc desc;
    vk::ShaderModule shaderModule;
    vk::ShaderStageFlagBits stageFlagBits{};

    // XXX: Need to implemenent shader specializations.

private:
    const VulkanContext& m_Context;
};

/*
 * XXX: Currently not implemented.
 * Used for shader specializations.
 */
class VulkanShaderLibrary : public RefCountResource<IShaderLibrary>
{
public:
    explicit VulkanShaderLibrary(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanShaderLibrary() override;

    ShaderHandle GetShader(const char* entryName, ShaderType type) override;
    void GetBytecode(const void** ppByteCode, size_t* pSize) const override;

    vk::ShaderModule shaderModule;

private:
    const VulkanContext& m_Context;
};

class VulkanInputLayout : public RefCountResource<IInputLayout>
{
public:
    u32 GetNumAttributes() const override;
    const VertexAttributeDesc* GetAttributeDesc(u32 index) const override;

    std::vector<VertexAttributeDesc> inputDesc;
    std::vector<vk::VertexInputBindingDescription> bindingDesc;
    std::vector<vk::VertexInputAttributeDescription> attributeDesc;
};

} // namespace Vulkan

} // namespace SuohRHI