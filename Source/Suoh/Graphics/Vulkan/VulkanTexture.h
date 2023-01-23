#pragma once

#include "VulkanCommon.h"

class VulkanDevice;

class Sampler : public GPUResource
{
public:
    Sampler(VulkanDevice* device, const SamplerDesc& createDesc);
    virtual ~Sampler();

    const SamplerDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Sampler GetVulkanHandle() const
    {
        return m_VulkanSampler;
    }

private:
    VulkanDevice* m_Device;
    SamplerDesc m_Desc;

    vk::Sampler m_VulkanSampler;

    vk::Filter m_MinFilter{vk::Filter::eLinear};
    vk::Filter m_MagFilter{vk::Filter::eLinear};

    vk::SamplerMipmapMode m_MipFilter{vk::SamplerMipmapMode::eLinear};

    vk::SamplerAddressMode m_AddressModeU{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode m_AddressModeV{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode m_AddressModeW{vk::SamplerAddressMode::eRepeat};

    vk::SamplerReductionMode m_ReductionMode{vk::SamplerReductionMode::eWeightedAverage};

    std::string m_Name;
};

class Texture : public GPUResource
{
public:
    Texture(VulkanDevice* device, const TextureDesc& desc);
    Texture(VulkanDevice* device, const TextureViewDesc& textureViewDesc);

    virtual ~Texture();

    void InitTextureView(const TextureViewDesc& desc);

    const TextureDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Image GetVulkanHandle() const
    {
        return m_VulkanImage;
    }

    vk::ImageView GetVulkanView() const
    {
        return m_VulkanImageView;
    }

    void SetResourceState(ResourceState state)
    {
        m_ResourceState = state;
    }

    u32 GetWidth() const
    {
        return m_Width;
    }

    u32 GetHeight() const
    {
        return m_Height;
    }

    u32 GetDepth() const
    {
        return m_Depth;
    }

    u32 GetMipLevelCount() const
    {
        return m_MipLevelCount;
    }

    u32 GetArrayLayerCount() const
    {
        return m_ArrayLayerCount;
    }

    VmaAllocation GetVmaAllocation() const
    {
        return m_VmaAllocation;
    }

    vk::Format GetFormat() const
    {
        return m_VulkanFormat;
    }

    TextureType GetType() const
    {
        return m_Type;
    }

    void LinkSampler(Sampler* sampler)
    {
        m_Sampler = sampler;
    }

    Sampler* GetSampler() const
    {
        return m_Sampler;
    }

private:
    // Creates a texture view of an already existing texture.
    void InitTextureViewFromExistingTexture(const TextureViewDesc& textureViewDesc);

private:
    VulkanDevice* m_Device;
    TextureDesc m_Desc;

    vk::Image m_VulkanImage;
    vk::ImageView m_VulkanImageView;
    vk::Format m_VulkanFormat;
    VmaAllocation m_VmaAllocation;

    ResourceState m_ResourceState{ResourceState::Undefined};

    u32 m_Width{1};
    u32 m_Height{1};
    u32 m_Depth{1};

    u32 m_ArrayLayerCount{1};
    u32 m_ArrayBaseLayer{0};
    u32 m_MipLevelCount{1};
    u32 m_MipBaseLevel{0};

    TextureFlags m_Flags;

    // XXX: Is a strong reference needed here?
    TextureRef m_ParentTexture;

    TextureType m_Type{TextureType::Texture2D};

    Sampler* m_Sampler{nullptr};
    // SamplerRef m_Sampler{nullptr};

    // vk::ImageLayout m_CurrentLayout;

    std::string m_Name;

    friend class VulkanDevice;
};
;
