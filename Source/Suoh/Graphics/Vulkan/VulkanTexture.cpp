#include "VulkanCommon.h"
#include "VulkanDevice.h"

#include <Core/Logger.h>

Sampler::Sampler(VulkanDevice* device, const SamplerDesc& desc) : m_Device(std::move(device))
{
    auto samplerInfo = vk::SamplerCreateInfo()
                           .setMinFilter(desc.minFilter)
                           .setMagFilter(desc.magFilter)
                           .setMipmapMode(desc.mipFilter)
                           .setAddressModeU(desc.addressModeU)
                           .setAddressModeV(desc.addressModeV)
                           .setAddressModeW(desc.addressModeW)
                           .setMipLodBias(1.0f)
                           .setAnisotropyEnable(false)
                           .setMaxAnisotropy(1)
                           .setCompareEnable(false)
                           .setCompareOp(vk::CompareOp::eAlways)
                           .setMinLod(0.0f)
                           .setMaxLod(16.0f)
                           .setBorderColor(vk::BorderColor::eIntOpaqueWhite)
                           .setUnnormalizedCoordinates(false);

    auto samplerReductionInfo = vk::SamplerReductionModeCreateInfoEXT();
    if (desc.reductionMode != vk::SamplerReductionMode::eWeightedAverage)
    {
        samplerReductionInfo.setReductionMode(desc.reductionMode);
        samplerInfo.setPNext(&samplerReductionInfo);
    }

    auto vkRes = m_Device->GetVulkanDevice().createSampler(&samplerInfo, m_Device->GetVulkanAllocationCallbacks(),
                                                           &m_VulkanSampler);
    if (vkRes != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create sampler!");
        return;
    }

    m_Device->SetVulkanResourceName(vk::ObjectType::eSampler, m_VulkanSampler, desc.name);

    m_AddressModeU = desc.addressModeU;
    m_AddressModeV = desc.addressModeV;
    m_AddressModeW = desc.addressModeW;
    m_MinFilter = desc.minFilter;
    m_MagFilter = desc.magFilter;
    m_MipFilter = desc.mipFilter;
    m_Name = desc.name;
    m_ReductionMode = desc.reductionMode;

    m_Desc = desc;
}

Sampler::~Sampler()
{
    if (m_VulkanSampler)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::Sampler, m_VulkanSampler);
    }
}

SamplerRef VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    auto samplerRef = SamplerRef::Create(new Sampler(this, desc));

    if (!samplerRef->GetVulkanHandle())
    {
        samplerRef = nullptr;
    }

    return samplerRef;
}

Texture::Texture(VulkanDevice* device, const TextureDesc& desc) : m_Device(std::move(device))
{
    auto imageInfo = vk::ImageCreateInfo()
                         .setFlags(vk::ImageCreateFlags(0))
                         .setImageType(ToVkImageType(desc.type))
                         .setFormat(desc.format)
                         .setExtent(vk::Extent3D().setWidth(desc.width).setHeight(desc.height).setDepth(desc.depth))
                         .setMipLevels(desc.mipLevelCount)
                         .setArrayLayers(desc.arrayLayerCount)
                         .setSamples(vk::SampleCountFlagBits::e1)
                         .setTiling(vk::ImageTiling::eOptimal)
                         .setUsage(vk::ImageUsageFlagBits::eSampled)
                         .setSharingMode(vk::SharingMode::eExclusive)
                         .setInitialLayout(vk::ImageLayout::eUndefined);

    const bool isRenderTarget = (desc.flags & TextureFlags::RenderTarget);
    const bool isCompute = (desc.flags & TextureFlags::Compute);
    const bool hasDepthOrStencil = HasDepthOrStencil(desc.format);

    // Always make copyable.
    imageInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    if (isCompute)
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eStorage;
    }

    if (isRenderTarget)
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
    }

    if (hasDepthOrStencil)
    {
        imageInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    }

    const auto vmaAllocator = m_Device->GetVmaAllocator();

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (desc.alias == nullptr)
    {
        auto res = vmaCreateImage(vmaAllocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocCreateInfo,
                                  reinterpret_cast<VkImage*>(&m_VulkanImage), &m_VmaAllocation, nullptr);
        if (res != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create vulkan image!");
            return;
        }

#if defined(VULKAN_DEBUG)
        vmaSetAllocationName(vmaAllocator, m_VmaAllocation, desc.name.c_str());
#endif
    }
    else
    {
        m_VmaAllocation = 0;
        auto res = vmaCreateAliasingImage(vmaAllocator, desc.alias->GetVmaAllocation(),
                                          reinterpret_cast<VkImageCreateInfo*>(&imageInfo),
                                          reinterpret_cast<VkImage*>(&m_VulkanImage));
    }

    m_Device->SetVulkanResourceName(vk::ObjectType::eImage, m_VulkanImage, desc.name);

    m_Width = desc.width;
    m_Height = desc.height;
    m_Depth = desc.depth;
    m_MipBaseLevel = 0;
    m_ArrayBaseLayer = 0;
    m_ArrayLayerCount = desc.arrayLayerCount;
    m_MipLevelCount = desc.mipLevelCount;
    m_Type = desc.type;
    m_Name = desc.name;
    m_VulkanFormat = desc.format;
    m_Sampler = nullptr;
    m_Flags = desc.flags;
    m_ParentTexture = nullptr;
    m_ResourceState = ResourceState::Undefined;

    TextureViewDesc textureViewDesc;
    textureViewDesc.name = desc.name;
    textureViewDesc.mipLevelCount = desc.mipLevelCount;
    textureViewDesc.arrayLayerCount = desc.arrayLayerCount;

    InitTextureView(textureViewDesc);

    if (m_Device->UseBindless())
    {
        ResourceUpdate resourceUpdate;
        resourceUpdate.type = ResourceType::Texture;
        resourceUpdate.resource = this;

        m_Device->AddBindlessResourceUpdate(resourceUpdate);
    }

    if (desc.initialData)
    {
        // XXX: TODO.
        // UploadTextureData(texture, desc.initialData);
    }
}

Texture::Texture(VulkanDevice* device, const TextureViewDesc& textureViewDesc) : m_Device(std::move(device))
{
    InitTextureViewFromExistingTexture(textureViewDesc);
}

Texture::~Texture()
{
    assert(m_ParentTexture != this);

    if (m_VulkanImageView)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::TextureView,
                                                             m_VulkanImageView);
    }

    if (!m_ParentTexture && m_VulkanImage)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueAllocatedResource(DeferredDeletionQueue::Type::TextureAllocated,
                                                                      m_VulkanImage, m_VmaAllocation);
    }
}

void Texture::InitTextureView(const TextureViewDesc& desc)
{
    if (m_VulkanImageView)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::TextureView,
                                                             m_VulkanImageView);
    }

    auto aspectFlags = vk::ImageAspectFlags();

    if (HasDepthOrStencil(m_VulkanFormat))
    {
        if (HasDepth(m_VulkanFormat))
        {
            aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        }

        if (HasStencil(m_VulkanFormat))
        {
            aspectFlags |= vk::ImageAspectFlagBits::eStencil;
        }
    }
    else
    {
        aspectFlags |= vk::ImageAspectFlagBits::eColor;
    }

    auto viewInfo = vk::ImageViewCreateInfo()
                        .setFlags(vk::ImageViewCreateFlags(0))
                        .setImage(m_VulkanImage)
                        .setViewType(ToVkImageViewType(m_Type))
                        .setFormat(m_VulkanFormat)
                        .setSubresourceRange(vk::ImageSubresourceRange()
                                                 .setAspectMask(aspectFlags)
                                                 .setBaseMipLevel(desc.mipBaseLevel)
                                                 .setLevelCount(desc.mipLevelCount)
                                                 .setBaseArrayLayer(desc.arrayBaseLayer)
                                                 .setLayerCount(desc.arrayLayerCount));
    auto vkRes = m_Device->GetVulkanDevice().createImageView(&viewInfo, m_Device->GetVulkanAllocationCallbacks(),
                                                             &m_VulkanImageView);
    VK_CHECK(vkRes);

    m_Device->SetVulkanResourceName(vk::ObjectType::eImageView, m_VulkanImageView, desc.name);
}

void Texture::InitTextureViewFromExistingTexture(const TextureViewDesc& desc)
{
    if (desc.parentTexture == nullptr)
    {
        LOG_ERROR("CreateTextureView: Invalid parent texture!");
        return;
    }

    m_ParentTexture = desc.parentTexture;

    // XXX: Copy complete texture info from parent texture.
    m_Desc = m_ParentTexture->GetDesc();

    m_VulkanImage = m_ParentTexture->GetVulkanHandle();

    m_VulkanFormat = m_ParentTexture->GetFormat();
    m_Type = m_ParentTexture->GetType();
    m_Width = m_ParentTexture->GetWidth();
    m_Height = m_ParentTexture->GetHeight();
    m_Depth = m_ParentTexture->GetDepth();

    m_MipBaseLevel = desc.mipBaseLevel;
    m_MipLevelCount = desc.mipLevelCount;
    m_ArrayBaseLayer = desc.arrayBaseLayer;
    m_ArrayLayerCount = desc.arrayLayerCount;

    InitTextureView(desc);
}

TextureRef VulkanDevice::CreateTexture(const TextureDesc& desc)
{
    auto textureRef = TextureRef::Create(new Texture(this, desc));

    if (!textureRef->GetVulkanHandle() || !textureRef->GetVulkanView())
    {
        textureRef = nullptr;
    }

    return textureRef;
}

TextureRef VulkanDevice::CreateTextureView(const TextureViewDesc& desc)
{
    assert(desc.parentTexture != nullptr);

    auto textureRef = TextureRef::Create(new Texture(this, desc));

    if (!textureRef->GetVulkanView())
    {
        textureRef = nullptr;
    }

    return textureRef;
}