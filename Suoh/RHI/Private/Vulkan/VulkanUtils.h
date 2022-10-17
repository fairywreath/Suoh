#pragma once

#include "VulkanCommon.h"

#include "VulkanTexture.h"

namespace SuohRHI
{

namespace Vulkan
{

template <typename T> T align(T size, T alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

template <typename T, typename U> [[nodiscard]] bool ArraysAreDifferent(const T& a, const U& b)
{
    if (a.size() != b.size())
        return true;

    for (u32 i = 0; i < u32(a.size()); i++)
    {
        if (a[i] != b[i])
            return true;
    }

    return false;
}

// XXX: Not Vulkan specific, move this somwhere.
// const FormatInfo& GetFormatInfo(Format format);
FormatInfo GetFormatInfo(Format format);

// RHI to Vulkan types converters.
vk::Format ToVkFormat(Format format);

/*
 * Texture helpers.
 */
void FillTextureInfo(VulkanTexture* texture, const TextureDesc& desc);
vk::ImageViewType VkImageViewTypeFromTextureDimension(TextureDimension dimension);
vk::ImageAspectFlags GuessImageAspectFlags(vk::Format format);
vk::ImageAspectFlags GuessSubresourceImageAspectFlags(vk::Format format, VulkanTexture::TextureSubresourceViewType viewType);

/*
 * Sampler helpers.
 */
vk::SamplerAddressMode ToSamplerAddressMode(TextureAddressMode addressMode);
vk::Filter ToMagFilter(SamplerFilter filter);
vk::Filter ToMinFilter(SamplerFilter filter);
vk::BorderColor ToSamplerBorderColor(StaticBorderColor borderColor);

/*
 * Shader helpers.
 */
vk::ShaderStageFlagBits ToVkShaderStageFlagBits(ShaderType type);

/*
 * Resource state/barriers helpers.
 */
VulkanResourceStateMapping ConvertResourceState(ResourceStates state);

/*
 * Binding helpers.
 */
VulkanTexture::TextureSubresourceViewType ToTextureViewType(Format bindingFormat, Format textureFormat);

/*
 * Framebuffer/renderPass/graphicsPipeline helpers.
 */
// MAX_RENDER_TARGETS = max number of color attachments + 1 depth attachment
template <typename T> using attachment_vector = static_vector<T, MAX_RENDER_TARGETS + 1>;
TextureDimension GetTextureDimensionForFramebuffer(TextureDimension dimension, bool isArray);

vk::PrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology);
vk::PolygonMode ToVkPolygonMode(FillMode mode);
vk::CullModeFlagBits ToVkCullMode(CullMode mode);
vk::CompareOp ToVkCompareOp(ComparisonFunc func);
vk::StencilOp ToVkStencilOp(StencilOp op);
vk::StencilOpState ConvertStencilState(const DepthStencilState& depthStencilState, const StencilOpDesc& desc);

vk::Viewport ToVkViewport(Viewport vp);

} // namespace Vulkan

} // namespace SuohRHI