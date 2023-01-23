#pragma once

#include <Core/Containers.h>

#include "GPUConstants.h"
#include "VulkanCommon.h"
#include "VulkanResources.h"

#include "VulkanBuffer.h"

class VulkanDevice;

struct DescriptorBinding
{
    vk::DescriptorType type;
    u32 index{u32(-1)};
    u32 count{0};
    vk::ShaderStageFlags shaderStageFlags;

    DescriptorBinding() = default;
    DescriptorBinding(vk::DescriptorType _type, u32 _index, u32 _count,
                      vk::ShaderStageFlags _shaderStageFlags = vk::ShaderStageFlagBits::eAll)
        : type(_type), index(_index), count(_count), shaderStageFlags(_shaderStageFlags)
    {
    }
};

using DescriptorBindingArray = static_vector<DescriptorBinding, GPUConstants::MaxDescriptorsPerSet>;

struct DescriptorSetLayoutDesc
{
    DescriptorBindingArray bindings{};

    u32 setIndex{u32(-1)};

    bool bindless{false};
    bool dynamic{false};

    std::string name;

    DescriptorSetLayoutDesc& Reset()
    {
        bindings = {};
        setIndex = u32(-1);
        bindless = false;
        dynamic = false;
        return *this;
    }
    DescriptorSetLayoutDesc& AddBinding(const DescriptorBinding& binding)
    {
        bindings.push_back(binding);
        return *this;
    }
    DescriptorSetLayoutDesc& AddBinding(vk::DescriptorType type, u32 index, u32 count,
                                        vk::ShaderStageFlags shaderFlags = vk::ShaderStageFlagBits::eAll)
    {
        bindings.push_back(DescriptorBinding(type, index, count, shaderFlags));
        return *this;
    }
    DescriptorSetLayoutDesc& SetName(const std::string& _name)
    {
        name = _name;
        return *this;
    }
    DescriptorSetLayoutDesc& SetSetIndex(u32 index)
    {
        setIndex = index;
        return *this;
    }
    DescriptorSetLayoutDesc& SetBindless(bool value)
    {
        bindless = value;
        return *this;
    }
    DescriptorSetLayoutDesc& SetDynamic(bool value)
    {
        dynamic = value;
        return *this;
    }
};

// Used to obtain index to binding data array given shader binding index.
using BindingDataIndicesArray = static_vector<u8, GPUConstants::MaxShaderBindingIndex>;

class DescriptorSetLayout : public GPUResource
{
public:
    DescriptorSetLayout(VulkanDevice* device, const DescriptorSetLayoutDesc& desc);
    ~DescriptorSetLayout();

    vk::DescriptorSetLayout GetVulkanHandle() const
    {
        return m_VulkanDescriptorSetLayout;
    }

    bool IsBindless() const
    {
        return m_Bindless;
    }

    bool IsDynamic() const
    {
        return m_Dynamic;
    }

    u32 NumBindings() const
    {
        return m_Bindings.size();
    }

    const std::vector<DescriptorBinding>& GetBindings() const
    {
        return m_Bindings;
    }

    const std::vector<vk::DescriptorSetLayoutBinding>& GetVulkanBindings() const
    {
        return m_VulkanBindings;
    }

    const BindingDataIndicesArray& GetBindingDataIndices() const
    {
        return m_BindingDataIndices;
    }

    u32 GetSetIndex() const
    {
        return m_SetIndex;
    }

private:
    VulkanDevice* m_Device;

    vk::DescriptorSetLayout m_VulkanDescriptorSetLayout;

    u32 m_SetIndex{0};

    bool m_Bindless{false};
    bool m_Dynamic{false};

    std::vector<vk::DescriptorSetLayoutBinding> m_VulkanBindings;
    std::vector<DescriptorBinding> m_Bindings;

    BindingDataIndicesArray m_BindingDataIndices;
};

struct DescriptorSetBindingResource
{
    enum class Type
    {
        Buffer,
        Texture,
        TextureArray,
        Undefined,
    };

    // XXX: Maybe only hold the views?
    std::variant<GPUResource*, std::vector<GPUResource*>> data{nullptr};
    Type type{Type::Undefined};
    u32 count{0};

    // Custom sampler for Textures.
    // Sampler* sampler{nullptr};
    DescriptorSetBindingResource() = default;
    DescriptorSetBindingResource(Type _type, GPUResource* _data, u32 _count) : type(_type), data(_data), count(_count)
    {
        assert(_type != Type::TextureArray);
    }
    DescriptorSetBindingResource(const std::vector<Texture*>& textures)
        : type(Type::TextureArray), count(textures.size())
    {
        auto& textureArray = get<std::vector<GPUResource*>>(data);
        textureArray.reserve(textures.size());
        for (const auto texture : textures)
        {
            textureArray.push_back(texture);
        }
    }

    static DescriptorSetBindingResource BufferResource(Buffer* buffer)
    {
        return DescriptorSetBindingResource(Type::Buffer, buffer, 1);
    }
    static DescriptorSetBindingResource TextureResource(Texture* texture)
    {
        return DescriptorSetBindingResource(Type::Texture, texture, 1);
    }
    static DescriptorSetBindingResource TextureArrayResource(const std::vector<Texture*>& textures)
    {
        return DescriptorSetBindingResource(textures);
    }
};

using DescriptorSetBindingResourceArray
    = static_vector<DescriptorSetBindingResource, GPUConstants::MaxDescriptorsPerSet>;
using ShaderBindingIndicesArray = static_vector<u8, GPUConstants::MaxDescriptorsPerSet>;

struct DescriptorSetDesc
{
    // std::vector<DescriptorSetResource> resources;
    // u32 numResources{0};
    DescriptorSetBindingResourceArray bindingResources;

    // Binding indices in the shader program.
    // std::vector<u32> shaderBindings;
    ShaderBindingIndicesArray shaderBindings;

    // Binding info is obtained from the layout.
    DescriptorSetLayout* descriptorSetLayout;

    u32 setIndex{0};

    std::string name;

    DescriptorSetDesc& Reset();
    DescriptorSetDesc& SetLayout(DescriptorSetLayoutRef _layout);
    DescriptorSetDesc& AddTexture(TextureRef texture, u32 binding);
    DescriptorSetDesc& AddBuffer(BufferRef buffer, u32 binding);
    DescriptorSetDesc& AddTextureSampler(TextureRef texture, SamplerRef sampler, u32 binding);
    DescriptorSetDesc& SetName(const std::string& _name);
    DescriptorSetDesc& SetSetIndex(u32 index);
};

class DescriptorSet : public GPUResource
{
public:
    DescriptorSet(VulkanDevice* device, const DescriptorSetDesc& desc);
    virtual ~DescriptorSet();

    vk::DescriptorSet GetVulkanHandle() const
    {
        return m_VulkanDescriptorSet;
    }

    DescriptorSetLayout* GetDescriptorSetLayout()
    {
        return m_Layout;
    }

    const DescriptorSetBindingResourceArray& GetResources() const
    {
        return m_BindingResources;
    }

    vk::PipelineStageFlags GetVulkanPipelineStageFlags() const
    {
        return m_VulkanPipelineStageFlags;
    }

private:
    VulkanDevice* m_Device;

    vk::DescriptorSet m_VulkanDescriptorSet;

    DescriptorSetBindingResourceArray m_BindingResources;
    ShaderBindingIndicesArray m_ShaderBindingIndices;

    vk::PipelineStageFlags m_VulkanPipelineStageFlags{vk::PipelineStageFlagBits::eAllGraphics};

    DescriptorSetLayoutRef m_Layout;
};

class VulkanDescriptorSetsManager
{
public:
    VulkanDescriptorSetsManager() = default;
    ~VulkanDescriptorSetsManager();

    NON_COPYABLE(VulkanDescriptorSetsManager);
    NON_MOVEABLE(VulkanDescriptorSetsManager);

    void Init(VulkanDevice* device);

    vk::DescriptorPool GetVulkanGlobalDescriptorPool() const
    {
        return m_VulkanGlobalDescriptorPool;
    }

    vk::DescriptorPool GetVulkanGlobalBindlessDescriptorPool() const
    {
        return m_VulkanGlobalBindlessDescriptorPool;
    }

private:
    void DestroyVulkanDescriptorPools();

private:
    VulkanDevice* m_Device{nullptr};

    vk::DescriptorPool m_VulkanGlobalDescriptorPool;
    vk::DescriptorPool m_VulkanGlobalBindlessDescriptorPool;
};