#include "VulkanTexture.h"
#include "VulkanUtils.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanTexture::~VulkanTexture()
{
    for (auto& p : subresourceViews)
    {
        auto& view = p.second.imageView;
        m_Context.device.destroyImageView(view);
        view = vk::ImageView();
    }
    subresourceViews.clear();

    if (managed)
    {
        if (image)
        {
            vmaDestroyImage(m_Context.allocator, image, allocation);
        }
    }
}

VulkanTextureSubresourceView& VulkanTexture::GetSubresourceView(const TextureSubresourceSet& subresources, TextureDimension dimension,
                                                                TextureSubresourceViewType viewtype)
{
    std::lock_guard lockGuard(m_Mutex);

    if (dimension == TextureDimension::UNKNOWN)
        dimension = desc.dimension;

    auto cachekey = std::make_tuple(subresources, viewtype, dimension);
    auto iter = subresourceViews.find(cachekey);
    if (iter != subresourceViews.end())
    {
        return iter->second;
    }

    auto iter_pair = subresourceViews.emplace(cachekey, *this);
    auto& view = std::get<0>(iter_pair)->second;

    view.subresource = subresources;

    auto vkformat = ToVkFormat(desc.format);

    vk::ImageAspectFlags aspectflags = GuessSubresourceImageAspectFlags(vkformat, viewtype);
    view.imageSubresourceRange = vk::ImageSubresourceRange()
                                     .setAspectMask(aspectflags)
                                     .setBaseMipLevel(subresources.baseMipLevel)
                                     .setLevelCount(subresources.numMipLevels)
                                     .setBaseArrayLayer(subresources.baseArrayLayer)
                                     .setLayerCount(subresources.numArrayLayers);

    vk::ImageViewType imageViewType = VkImageViewTypeFromTextureDimension(dimension);

    auto viewInfo = vk::ImageViewCreateInfo()
                        .setImage(image)
                        .setViewType(imageViewType)
                        .setFormat(vkformat)
                        .setSubresourceRange(view.imageSubresourceRange);

    if (viewtype == TextureSubresourceViewType::STENCIL_ONLY)
    {
        // HLSL puts stencil values in second component.Imitate that here.
        viewInfo.components.setG(vk::ComponentSwizzle::eR);
    }

    const vk::Result res = m_Context.device.createImageView(&viewInfo, nullptr, &view.imageView);
    VK_ASSERT_OK(res);

    // XXX: set debug utils names?

    return view;
}

u32 VulkanTexture::GetNumSubresources() const
{
    return desc.mipLevels * desc.layerCount;
}

u32 VulkanTexture::GetSubresourceIndex(u32 mipLevel, u32 arrayLayer) const
{
    return mipLevel * desc.layerCount + arrayLayer;
}

Object VulkanTexture::GetNativeObject(ObjectType objectType)
{
    switch (objectType)
    {

    case ObjectTypes::Vk_Image:
        return Object(image);
    default:
        return Object(nullptr);
    }
}

Object VulkanTexture::GetNativeView(ObjectType objectType, Format format, TextureSubresourceSet subresources, TextureDimension dimension)
{
    switch (objectType)
    {
    case ObjectTypes::Vk_ImageView: {
        if (format == Format::UNKNOWN)
            format = desc.format;

        const FormatInfo& formatInfo = GetFormatInfo(format);

        TextureSubresourceViewType viewType = TextureSubresourceViewType::ALL_ASPECTS;
        if (formatInfo.hasDepth && !formatInfo.hasStencil)
            viewType = TextureSubresourceViewType::DEPTH_ONLY;
        else if (!formatInfo.hasDepth && formatInfo.hasStencil)
            viewType = TextureSubresourceViewType::STENCIL_ONLY;

        return Object(GetSubresourceView(subresources, dimension, viewType).imageView);
    }
    default:
        return Object(nullptr);
    }
}

VulkanSampler::~VulkanSampler()
{
    m_Context.device.destroySampler(sampler);
}

Object VulkanSampler::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_Sampler:
        return Object(sampler);
    default:
        return Object(nullptr);
    }
}

static i64 AlignBufferOffset(i64 off)
{
    static constexpr i64 bufferAlignmentBytes = 4;
    return ((off + (bufferAlignmentBytes - 1)) / bufferAlignmentBytes) * bufferAlignmentBytes;
}

size_t VulkanStagingTexture::ComputeLayerSize(u32 mipLevel)
{
    const FormatInfo& formatInfo = GetFormatInfo(desc.format);

    auto wInBlocks = (desc.width >> mipLevel) / formatInfo.blockSize;
    auto hInBlocks = (desc.height >> mipLevel) / formatInfo.blockSize;

    auto blockPitchBytes = (wInBlocks >> mipLevel) * formatInfo.bytesPerBlock;
    return blockPitchBytes * hInBlocks;
}

const StagingTextureRegion& VulkanStagingTexture::GetLayerRegion(u32 mipLevel, u32 arrayLayer, u32 z)
{
    if (desc.depth != 1)
    {
        // Hard case, since each mip level has half the slices as the previous one.
        assert(arrayLayer == 0);
        assert(z < desc.depth);

        u32 mipDepth = desc.depth;
        u32 index = 0;
        while (mipLevel-- > 0)
        {
            index += mipDepth;
            mipDepth = std::max(mipDepth, u32(1));
        }
        return layerRegions[index + z];
    }
    else if (desc.layerCount != 1)
    {
        // Easy case, since each mip level has a consistent number of slices.
        assert(z == 0);
        assert(arrayLayer < desc.layerCount);
        assert(layerRegions.size() == desc.mipLevels * desc.layerCount);
        return layerRegions[mipLevel * desc.layerCount + arrayLayer];
    }
    else
    {
        assert(arrayLayer == 0);
        assert(z == 0);
        assert(layerRegions.size() == 1);
        return layerRegions[0];
    }
}

void VulkanStagingTexture::PopulateLayerRegions()
{
    i64 curOffset = 0;

    layerRegions.clear();

    for (u32 mip = 0; mip < desc.mipLevels; mip++)
    {
        auto layerSize = ComputeLayerSize(mip);

        u32 depth = std::max(desc.depth >> mip, u32(1));
        u32 numSlices = desc.layerCount * depth;

        for (u32 slice = 0; slice < numSlices; slice++)
        {
            layerRegions.push_back({curOffset, layerSize});

            // update offset for the next region
            curOffset = AlignBufferOffset(i64(curOffset + layerSize));
        }
    }
}

} // namespace Vulkan

} // namespace SuohRHI