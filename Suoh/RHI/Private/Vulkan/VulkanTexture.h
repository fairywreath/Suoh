#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"

#include "CommandListStateTracker.h"

#include <unordered_map>

namespace SuohRHI
{

namespace Vulkan
{

class VulkanTexture;

// Wraps  VkImageView.
struct VulkanTextureSubresourceView
{
    VulkanTextureSubresourceView(VulkanTexture& texture) : texture(texture)
    {
    }

    NON_COPYABLE(VulkanTextureSubresourceView);

    bool operator==(const VulkanTextureSubresourceView& other) const
    {
        return (&texture == &other.texture) && (subresource == other.subresource) && (imageView == other.imageView)
               && (imageSubresourceRange == other.imageSubresourceRange);
    }

    VulkanTexture& texture;
    TextureSubresourceSet subresource;

    vk::ImageView imageView;
    vk::ImageSubresourceRange imageSubresourceRange;
};

class VulkanTexture : public RefCountResource<ITexture>, public TextureStateTracker
{
public:
    explicit VulkanTexture(const VulkanContext& context) : TextureStateTracker(desc), m_Context(context)
    {
    }
    ~VulkanTexture() override;

    enum class TextureSubresourceViewType
    {
        ALL_ASPECTS,
        DEPTH_ONLY,
        STENCIL_ONLY,
    };

    struct Hash
    {
        std::size_t operator()(std::tuple<TextureSubresourceSet, TextureSubresourceViewType, TextureDimension> const& s) const noexcept
        {
            const auto& [subresources, viewType, dimension] = s;

            size_t hash = 0;

            hash_combine(hash, subresources.baseMipLevel);
            hash_combine(hash, subresources.numMipLevels);
            hash_combine(hash, subresources.baseArrayLayer);
            hash_combine(hash, subresources.numArrayLayers);
            hash_combine(hash, viewType);
            hash_combine(hash, dimension);

            return hash;
        }
    };

    TextureDesc desc;

    vk::ImageCreateInfo imageInfo;
    vk::Image image;
    VmaAllocation allocation;

    bool managed{true};

    std::unordered_map<std::tuple<TextureSubresourceSet, TextureSubresourceViewType, TextureDimension>, VulkanTextureSubresourceView,
                       VulkanTexture::Hash>
        subresourceViews;

    // Genereate and get image view.
    VulkanTextureSubresourceView& GetSubresourceView(const TextureSubresourceSet& subresources, TextureDimension dimension,
                                                     TextureSubresourceViewType viewtype = TextureSubresourceViewType::ALL_ASPECTS);

    u32 GetNumSubresources() const;
    u32 GetSubresourceIndex(u32 mipLevel, u32 arrayLayer) const;

    const TextureDesc& GetDesc() const override
    {
        return desc;
    }
    Object GetNativeObject(ObjectType objectType) override;
    Object GetNativeView(ObjectType objectType, Format format, TextureSubresourceSet subresources, TextureDimension dimension) override;

private:
    const VulkanContext& m_Context;

    std::mutex m_Mutex;
};

/*
 * Staging textures.
 */
struct StagingTextureRegion
{
    i64 offset;
    size_t size;
};

class VulkanStagingTexture : public RefCountResource<IStagingTexture>
{
public:
    // XXX: Have destructor to free up staging internal buffer on destruction?

    const TextureDesc& GetDesc() const override
    {
        return desc;
    }

    TextureDesc desc;

    // Store if texture is buffer.
    RefCountPtr<VulkanBuffer> buffer;

    std::vector<StagingTextureRegion> layerRegions;

    size_t ComputeLayerSize(u32 mipLevel);
    const StagingTextureRegion& GetLayerRegion(u32 mipLevel, u32 arrayLayer, u32 z);
    void PopulateLayerRegions();

    size_t GetBufferSize()
    {
        assert(layerRegions.size());
        size_t size = layerRegions.back().offset + layerRegions.back().size;
        assert(size > 0);
        return size;
    }
};

/*
 * Sampler.
 */
class VulkanSampler : public RefCountResource<ISampler>
{
public:
    explicit VulkanSampler(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanSampler() override;

    const SamplerDesc& GetDesc() const override
    {
        return desc;
    }
    Object GetNativeObject(ObjectType type) override;

    SamplerDesc desc;
    vk::SamplerCreateInfo samplerInfo;
    vk::Sampler sampler;

private:
    const VulkanContext& m_Context;
};

} // namespace Vulkan

} // namespace SuohRHI
