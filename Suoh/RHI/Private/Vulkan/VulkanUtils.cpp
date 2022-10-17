#include "VulkanUtils.h"

#include "VulkanTexture.h"

#include <unordered_map>

namespace SuohRHI
{

namespace Vulkan
{

// XXX: Have the enum formats ordered and make this an array for fast access
static const std::unordered_map<SuohRHI::Format, vk::Format> C_FORMAT_MAP = {{
    {Format::UNKNOWN, vk::Format(VK_FORMAT_UNDEFINED)},
    {Format::R8_UINT, vk::Format(VK_FORMAT_R8_UINT)},
    {Format::R8_SINT, vk::Format(VK_FORMAT_R8_SINT)},
    {Format::R8_UNORM, vk::Format(VK_FORMAT_R8_UNORM)},
    {Format::R8_SNORM, vk::Format(VK_FORMAT_R8_SNORM)},
    {Format::R8G8_UINT, vk::Format(VK_FORMAT_R8G8_UINT)},
    {Format::R8G8_SINT, vk::Format(VK_FORMAT_R8G8_SINT)},
    {Format::R8G8_UNORM, vk::Format(VK_FORMAT_R8G8_UNORM)},
    {Format::R8G8_SNORM, vk::Format(VK_FORMAT_R8G8_SNORM)},
    {Format::R16_UINT, vk::Format(VK_FORMAT_R16_UINT)},
    {Format::R16_SINT, vk::Format(VK_FORMAT_R16_SINT)},
    {Format::R16_UNORM, vk::Format(VK_FORMAT_R16_UNORM)},
    {Format::R16_SNORM, vk::Format(VK_FORMAT_R16_SNORM)},
    {Format::R16_FLOAT, vk::Format(VK_FORMAT_R16_SFLOAT)},
    //{Format::B4G4R4A4_UNORM, vk::Format(VK_FORMAT_B4G4R4A4_UNORM_PACK16)},
    //{Format::B5G6R5_UNORM, vk::Format(VK_FORMAT_B5G6R5_UNORM_PACK16)},
    //{Format::B5G5R5A1_UNORM, vk::Format(VK_FORMAT_B5G5R5A1_UNORM_PACK16)},
    {Format::R8G8B8A8_UINT, vk::Format(VK_FORMAT_R8G8B8A8_UINT)},
    {Format::R8G8B8A8_SINT, vk::Format(VK_FORMAT_R8G8B8A8_SINT)},
    {Format::R8G8B8A8_UNORM, vk::Format(VK_FORMAT_R8G8B8A8_UNORM)},
    {Format::R8G8B8A8_SNORM, vk::Format(VK_FORMAT_R8G8B8A8_SNORM)},
    {Format::B8G8R8A8_UNORM, vk::Format(VK_FORMAT_B8G8R8A8_UNORM)},
    {Format::R8G8B8A8_UNORM, vk::Format(VK_FORMAT_R8G8B8A8_SRGB)},
    {Format::B8G8R8A8_UNORM, vk::Format(VK_FORMAT_B8G8R8A8_SRGB)},
    {Format::R10G10B10A2_UNORM, vk::Format(VK_FORMAT_A2B10G10R10_UNORM_PACK32)},
    {Format::R11G11B10_FLOAT, vk::Format(VK_FORMAT_B10G11R11_UFLOAT_PACK32)},
    {Format::R16G16_UINT, vk::Format(VK_FORMAT_R16G16_UINT)},
    {Format::R16G16_SINT, vk::Format(VK_FORMAT_R16G16_SINT)},
    {Format::R16G16_UNORM, vk::Format(VK_FORMAT_R16G16_UNORM)},
    {Format::R16G16_SNORM, vk::Format(VK_FORMAT_R16G16_SNORM)},
    {Format::R16G16_FLOAT, vk::Format(VK_FORMAT_R16G16_SFLOAT)},
    {Format::R32_UINT, vk::Format(VK_FORMAT_R32_UINT)},
    {Format::R32_SINT, vk::Format(VK_FORMAT_R32_SINT)},
    {Format::R32_FLOAT, vk::Format(VK_FORMAT_R32_SFLOAT)},
    {Format::R16G16B16A16_UINT, vk::Format(VK_FORMAT_R16G16B16A16_UINT)},
    {Format::R16G16B16A16_SINT, vk::Format(VK_FORMAT_R16G16B16A16_SINT)},
    {Format::R16G16B16A16_FLOAT, vk::Format(VK_FORMAT_R16G16B16A16_SFLOAT)},
    {Format::R16G16B16A16_UNORM, vk::Format(VK_FORMAT_R16G16B16A16_UNORM)},
    {Format::R16G16B16A16_SNORM, vk::Format(VK_FORMAT_R16G16B16A16_SNORM)},
    {Format::R32G32_UINT, vk::Format(VK_FORMAT_R32G32_UINT)},
    {Format::R32G32_SINT, vk::Format(VK_FORMAT_R32G32_SINT)},
    {Format::R32G32_FLOAT, vk::Format(VK_FORMAT_R32G32_SFLOAT)},
    {Format::R32G32B32_UINT, vk::Format(VK_FORMAT_R32G32B32_UINT)},
    {Format::R32G32B32_SINT, vk::Format(VK_FORMAT_R32G32B32_SINT)},
    {Format::R32G32B32_FLOAT, vk::Format(VK_FORMAT_R32G32B32_SFLOAT)},
    {Format::R32G32B32A32_UINT, vk::Format(VK_FORMAT_R32G32B32A32_UINT)},
    {Format::R32G32B32A32_SINT, vk::Format(VK_FORMAT_R32G32B32A32_SINT)},
    {Format::R32G32B32A32_FLOAT, vk::Format(VK_FORMAT_R32G32B32A32_SFLOAT)},
    {Format::D16_UNORM, vk::Format(VK_FORMAT_D16_UNORM)},
    {Format::D24_UNORM_S8_UINT, vk::Format(VK_FORMAT_D24_UNORM_S8_UINT)},
    {Format::D32_SFLOAT, vk::Format(VK_FORMAT_D32_SFLOAT)},
    {Format::D32_SFLOAT_S8X24_UINT, vk::Format(VK_FORMAT_D32_SFLOAT_S8_UINT)},
    //{Format::BC1_UNORM, vk::Format(VK_FORMAT_BC1_RGB_UNORM_BLOCK)},
    //{Format::BC1_UNORM_SRGB, vk::Format(VK_FORMAT_BC1_RGB_SRGB_BLOCK)},
    //{Format::BC2_UNORM, vk::Format(VK_FORMAT_BC2_UNORM_BLOCK)},
    //{Format::BC2_UNORM_SRGB, vk::Format(VK_FORMAT_BC2_SRGB_BLOCK)},
    //{Format::BC3_UNORM, vk::Format(VK_FORMAT_BC3_UNORM_BLOCK)},
    //{Format::BC3_UNORM_SRGB, vk::Format(VK_FORMAT_BC3_SRGB_BLOCK)},
    //{Format::BC4_UNORM, vk::Format(VK_FORMAT_BC4_UNORM_BLOCK)},
    //{Format::BC4_SNORM, vk::Format(VK_FORMAT_BC4_SNORM_BLOCK)},
    //{Format::BC5_UNORM, vk::Format(VK_FORMAT_BC5_UNORM_BLOCK)},
    //{Format::BC5_SNORM, vk::Format(VK_FORMAT_BC5_SNORM_BLOCK)},
    //{Format::BC6H_UFLOAT, vk::Format(VK_FORMAT_BC6H_UFLOAT_BLOCK)},
    //{Format::BC6H_SFLOAT, vk::Format(VK_FORMAT_BC6H_SFLOAT_BLOCK)},
    //{Format::BC7_UNORM, vk::Format(VK_FORMAT_BC7_UNORM_BLOCK)},
    //{Format::BC7_UNORM_SRGB, vk::Format(VK_FORMAT_BC7_SRGB_BLOCK)},

}};

static const VulkanResourceStateMapping C_RESOURCE_STATE_MAP[] = {
    {ResourceStates::COMMON, vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlags(), vk::ImageLayout::eUndefined},
    {ResourceStates::CONSTANT_BUFFER, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eUniformRead,
     vk::ImageLayout::eUndefined},
    {ResourceStates::VERTEX_BUFFER, vk::PipelineStageFlagBits::eVertexInput, vk::AccessFlagBits::eVertexAttributeRead,
     vk::ImageLayout::eUndefined},
    {ResourceStates::INDEX_BUFFER, vk::PipelineStageFlagBits::eVertexInput, vk::AccessFlagBits::eIndexRead, vk::ImageLayout::eUndefined},
    {ResourceStates::INDIRECT_ARGUMENT, vk::PipelineStageFlagBits::eDrawIndirect, vk::AccessFlagBits::eIndirectCommandRead,
     vk::ImageLayout::eUndefined},
    {ResourceStates::SHADER_RESOURCE, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eShaderRead,
     vk::ImageLayout::eShaderReadOnlyOptimal},
    {ResourceStates::UNORDERED_ACCESS, vk::PipelineStageFlagBits::eAllCommands,
     vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eGeneral},
    {ResourceStates::RENDER_TARGET, vk::PipelineStageFlagBits::eColorAttachmentOutput,
     vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal},
    {ResourceStates::DEPTH_WRITE, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
     vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
     vk::ImageLayout::eDepthStencilAttachmentOptimal},
    {ResourceStates::DEPTH_READ, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
     vk::AccessFlagBits::eDepthStencilAttachmentRead, vk::ImageLayout::eDepthStencilAttachmentOptimal},
    {ResourceStates::STREAM_OUT, vk::PipelineStageFlagBits::eTransformFeedbackEXT, vk::AccessFlagBits::eTransformFeedbackWriteEXT,
     vk::ImageLayout::eUndefined},
    {ResourceStates::COPY_DEST, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite,
     vk::ImageLayout::eTransferDstOptimal},
    {ResourceStates::COPY_SOURCE, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead,
     vk::ImageLayout::eTransferSrcOptimal},
    {ResourceStates::RESOLVE_DEST, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite,
     vk::ImageLayout::eTransferDstOptimal},
    {ResourceStates::RESOLVE_SOURCE, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead,
     vk::ImageLayout::eTransferSrcOptimal},
    {ResourceStates::PRESENT, vk::PipelineStageFlagBits::eAllCommands, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::ePresentSrcKHR},
    {ResourceStates::SHADING_RATE_SURFACE, vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR,
     vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR, vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR},
};

FormatInfo GetFormatInfo(Format format)
{
    // XXX: Properly handle this.
    return FormatInfo();
}

vk::Format ToVkFormat(Format format)
{
    return C_FORMAT_MAP.at(format);
}

vk::ImageType VkImageTypeFromTextureDimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::TEXTURE_1D:
    case TextureDimension::TEXTURE_1D_ARRAY:
        return vk::ImageType::e1D;

    case TextureDimension::TEXTURE_2D:
    case TextureDimension::TEXTURE_2D_ARRAY:
    case TextureDimension::TEXTURE_CUBE:
    case TextureDimension::TEXTURE_CUBE_ARRAY:
    case TextureDimension::TEXTURE_2DMS:
    case TextureDimension::TEXTURE_2DMS_ARRAY:
        return vk::ImageType::e2D;

    case TextureDimension::TEXTURE_3D:
        return vk::ImageType::e3D;

    case TextureDimension::UNKNOWN:
    default:
        return vk::ImageType::e2D;
    }
}

vk::ImageViewType VkImageViewTypeFromTextureDimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::TEXTURE_1D:
        return vk::ImageViewType::e1D;

    case TextureDimension::TEXTURE_1D_ARRAY:
        return vk::ImageViewType::e1DArray;

    case TextureDimension::TEXTURE_2D:
    case TextureDimension::TEXTURE_2DMS:
        return vk::ImageViewType::e2D;

    case TextureDimension::TEXTURE_2D_ARRAY:
    case TextureDimension::TEXTURE_2DMS_ARRAY:
        return vk::ImageViewType::e2DArray;

    case TextureDimension::TEXTURE_CUBE:
        return vk::ImageViewType::eCube;

    case TextureDimension::TEXTURE_CUBE_ARRAY:
        return vk::ImageViewType::eCubeArray;

    case TextureDimension::TEXTURE_3D:
        return vk::ImageViewType::e3D;

    case TextureDimension::UNKNOWN:
    default:
        return vk::ImageViewType::e2D;
    }
}

vk::Extent3D VkImageExtentFromTextureDesc(const TextureDesc& d)
{
    return vk::Extent3D(d.width, d.height, d.depth);
}

u32 ImageLayersFromTextureDesc(const TextureDesc& d)
{
    return d.layerCount;
}

vk::ImageUsageFlags VkImageUsageFlagsFromDesc(const TextureDesc& d)
{
    // XXX: Properly implement GetFormatInfo
    // const FormatInfo& formatInfo = GetFormatInfo(d.format);

    vk::ImageUsageFlags ret
        = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

    if (d.isRenderTarget)
    {
        // if (formatInfo.hasDepth || formatInfo.hasStencil)
        //{
        //     ret |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        // }
        // else
        //{
        //     ret |= vk::ImageUsageFlagBits::eColorAttachment;
        // }
        if (d.hasDepth || d.hasStencil)
        {
            ret |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        }
        else
        {
            ret |= vk::ImageUsageFlagBits::eColorAttachment;
        }
    }

    if (d.isStorage)
        ret |= vk::ImageUsageFlagBits::eStorage;

    // if (d.isShadingRateSurface)
    //     ret |= vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR;

    return ret;
}

vk::ImageCreateFlagBits VkImageCreateFlagsFromDesc(const TextureDesc& d)
{
    switch (d.dimension)
    {
    case TextureDimension::TEXTURE_CUBE:
    case TextureDimension::TEXTURE_CUBE_ARRAY:
        return vk::ImageCreateFlagBits::eCubeCompatible;

    case TextureDimension::TEXTURE_2D_ARRAY:
    case TextureDimension::TEXTURE_2DMS_ARRAY:
    case TextureDimension::TEXTURE_1D_ARRAY:
    case TextureDimension::TEXTURE_1D:
    case TextureDimension::TEXTURE_2D:
    case TextureDimension::TEXTURE_3D:
    case TextureDimension::TEXTURE_2DMS:
        return (vk::ImageCreateFlagBits)0;

    case TextureDimension::UNKNOWN:
    default:
        return (vk::ImageCreateFlagBits)0;
    }
}

vk::SampleCountFlagBits VkSampleCountFromDesc(const TextureDesc& d)
{
    switch (d.sampleCount)
    {
    case 1:
        return vk::SampleCountFlagBits::e1;

    case 2:
        return vk::SampleCountFlagBits::e2;

    case 4:
        return vk::SampleCountFlagBits::e4;

    case 8:
        return vk::SampleCountFlagBits::e8;

    case 16:
        return vk::SampleCountFlagBits::e16;

    case 32:
        return vk::SampleCountFlagBits::e32;

    case 64:
        return vk::SampleCountFlagBits::e64;

    default:
        return vk::SampleCountFlagBits::e1;
    }
}

vk::ImageAspectFlags GuessImageAspectFlags(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
        return vk::ImageAspectFlagBits::eDepth;

    case vk::Format::eS8Uint:
        return vk::ImageAspectFlagBits::eStencil;

    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

    default:
        return vk::ImageAspectFlagBits::eColor;
    }
}

vk::ImageAspectFlags GuessSubresourceImageAspectFlags(vk::Format format, VulkanTexture::TextureSubresourceViewType viewType)
{
    vk::ImageAspectFlags flags = GuessImageAspectFlags(format);
    if ((flags & (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil))
        == (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil))
    {
        if (viewType == VulkanTexture::TextureSubresourceViewType::DEPTH_ONLY)
        {
            flags = flags & (~vk::ImageAspectFlagBits::eStencil);
        }
        else if (viewType == VulkanTexture::TextureSubresourceViewType::STENCIL_ONLY)
        {
            flags = flags & (~vk::ImageAspectFlagBits::eDepth);
        }
    }
    return flags;
}

void FillTextureInfo(VulkanTexture* texture, const TextureDesc& desc)
{
    texture->desc = desc;

    vk::ImageType type = VkImageTypeFromTextureDimension(desc.dimension);
    vk::Extent3D extent = VkImageExtentFromTextureDesc(desc);
    u32 numLayers = ImageLayersFromTextureDesc(desc);
    vk::Format format = vk::Format(ToVkFormat(desc.format));
    vk::ImageUsageFlags usage = VkImageUsageFlagsFromDesc(desc);
    vk::SampleCountFlagBits sampleCount = VkSampleCountFromDesc(desc);
    vk::ImageCreateFlagBits flags = VkImageCreateFlagsFromDesc(desc);

    texture->imageInfo = vk::ImageCreateInfo()
                             .setImageType(type)
                             .setExtent(extent)
                             .setMipLevels(desc.mipLevels)
                             .setArrayLayers(numLayers)
                             .setFormat(format)
                             .setInitialLayout(vk::ImageLayout::eUndefined)
                             .setUsage(usage)
                             .setSharingMode(vk::SharingMode::eExclusive)
                             .setSamples(sampleCount)
                             .setFlags(flags);
}

vk::SamplerAddressMode ToSamplerAddressMode(TextureAddressMode addressMode)
{
    switch (addressMode)
    {
    case TextureAddressMode::WRAP:
        return vk::SamplerAddressMode::eRepeat;
    case TextureAddressMode::MIRROR:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case TextureAddressMode::CLAMP:
        return vk::SamplerAddressMode::eClampToEdge;
    case TextureAddressMode::BORDER:
        return vk::SamplerAddressMode::eClampToBorder;
    case TextureAddressMode::MIRROR_ONCE:
        return vk::SamplerAddressMode::eMirrorClampToEdge;
    default:
        // LOG_ERROR("ToSamplerAddresMode: Invalid enum ", addressMode);
        return vk::SamplerAddressMode::eRepeat;
    }
}

vk::Filter ToMagFilter(SamplerFilter filter)
{
    switch (filter)
    {
    case SamplerFilter::MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_ANISOTROPIC:
        return vk::Filter::eLinear;
    default:
        LOG_ERROR("ToMagFilter: Invalid enum!");
        return vk::Filter::eLinear;
    }
}

vk::Filter ToMinFilter(SamplerFilter filter)
{
    switch (filter)
    {
    case SamplerFilter::MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::COMPARISON_ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MINIMUM_ANISOTROPIC:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_MAG_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
        return vk::Filter::eNearest;
    case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_MIN_MAG_MIP_LINEAR:
        return vk::Filter::eLinear;
    case SamplerFilter::MAXIMUM_ANISOTROPIC:
        return vk::Filter::eLinear;
    default:
        LOG_ERROR("ToMinFilter: Invalid enum!");
        return vk::Filter::eNearest;
    }
}

vk::BorderColor ToSamplerBorderColor(StaticBorderColor borderColor)
{
    switch (borderColor)
    {
    case StaticBorderColor::TRANSPARENT_BLACK:
        return vk::BorderColor::eFloatTransparentBlack;
    case StaticBorderColor::OPAQUE_BLACK:
        return vk::BorderColor::eFloatOpaqueBlack;
    case StaticBorderColor::OPAQUE_WHITE:
        return vk::BorderColor::eFloatOpaqueWhite;
    default:
        // LOG_ERROR("ToSamplerBorderColor: Invalid enum ", borderColor);
        return vk::BorderColor::eFloatTransparentBlack;
    }
}

vk::ShaderStageFlagBits ToVkShaderStageFlagBits(ShaderType shaderType)
{
    if (shaderType == ShaderType::ALL)
        return vk::ShaderStageFlagBits::eAll;

    u32 result = 0;

    if ((shaderType & ShaderType::COMPUTE) != 0)
        result |= u32(vk::ShaderStageFlagBits::eCompute);
    if ((shaderType & ShaderType::VERTEX) != 0)
        result |= u32(vk::ShaderStageFlagBits::eVertex);
    if ((shaderType & ShaderType::TESSELLATION_CONTROL) != 0)
        result |= u32(vk::ShaderStageFlagBits::eTessellationControl);
    if ((shaderType & ShaderType::TESSELLATION_EVALUATION) != 0)
        result |= u32(vk::ShaderStageFlagBits::eTessellationEvaluation);
    if ((shaderType & ShaderType::GEOMETRY) != 0)
        result |= u32(vk::ShaderStageFlagBits::eGeometry);
    if ((shaderType & ShaderType::FRAGMENT) != 0)
        result |= u32(vk::ShaderStageFlagBits::eFragment);

    return vk::ShaderStageFlagBits(result);
}

VulkanResourceStateMapping ConvertResourceState(ResourceStates state)
{
    VulkanResourceStateMapping result = {};

    constexpr u32 numStateBits = sizeof(C_RESOURCE_STATE_MAP) / sizeof(C_RESOURCE_STATE_MAP[0]);

    u32 stateTmp = u32(state);
    u32 bitIndex = 0;

    while (stateTmp != 0 && bitIndex < numStateBits)
    {
        u32 bit = (1 << bitIndex);

        if (stateTmp & bit)
        {
            const VulkanResourceStateMapping& mapping = C_RESOURCE_STATE_MAP[bitIndex];

            assert(u32(mapping.rhiState) == bit);
            assert(result.imageLayout == vk::ImageLayout::eUndefined || mapping.imageLayout == vk::ImageLayout::eUndefined
                   || result.imageLayout == mapping.imageLayout);

            result.rhiState = ResourceStates(result.rhiState | mapping.rhiState);
            result.accessMask |= mapping.accessMask;
            result.stageFlags |= mapping.stageFlags;
            if (mapping.imageLayout != vk::ImageLayout::eUndefined)
                result.imageLayout = mapping.imageLayout;

            stateTmp &= ~bit;
        }

        bitIndex++;
    }

    assert(result.rhiState == state);

    return result;
}

VulkanTexture::TextureSubresourceViewType ToTextureViewType(Format bindingFormat, Format textureFormat)
{
    Format format = (bindingFormat == Format::UNKNOWN) ? textureFormat : bindingFormat;

    const auto& formatInfo = GetFormatInfo(format);

    if (formatInfo.hasDepth)
        return VulkanTexture::TextureSubresourceViewType::DEPTH_ONLY;
    if (formatInfo.hasStencil)
        return VulkanTexture::TextureSubresourceViewType::STENCIL_ONLY;

    return VulkanTexture::TextureSubresourceViewType::ALL_ASPECTS;
}

TextureDimension GetTextureDimensionForFramebuffer(TextureDimension dimension, bool isArray)
{
    // Convert 3D to 2D array.
    if (dimension == TextureDimension::TEXTURE_CUBE || dimension == TextureDimension::TEXTURE_CUBE_ARRAY
        || dimension == TextureDimension::TEXTURE_3D)
        dimension = TextureDimension::TEXTURE_2D_ARRAY;

    if (!isArray)
    {

        // Convert 2D to 1D if not array.
        switch (dimension)
        {
        case TextureDimension::TEXTURE_1D_ARRAY:
            dimension = TextureDimension::TEXTURE_1D;
            break;
        case TextureDimension::TEXTURE_2D_ARRAY:
            dimension = TextureDimension::TEXTURE_2D;
            break;
        case TextureDimension::TEXTURE_2DMS_ARRAY:
            dimension = TextureDimension::TEXTURE_2DMS;
            break;
        default:
            break;
        }
    }

    return dimension;
}

vk::PrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::TRIANGLES:
        return vk::PrimitiveTopology::eTriangleList;
    case PrimitiveTopology::TRIANGLE_STRIP:
        return vk::PrimitiveTopology::eTriangleStrip;
    default:
        LOG_ERROR("ToVkPrimitiveTopology: invalid enum!");
    }

    return vk::PrimitiveTopology();
}

vk::PolygonMode ToVkPolygonMode(FillMode mode)
{
    switch (mode)
    {
    case FillMode::SOLID:
        return vk::PolygonMode::eFill;
    case FillMode::WIREFRAME:
        return vk::PolygonMode::eLine;
    default:
        LOG_ERROR("ToVkPolygonMode: invalid enum!");
    }

    return vk::PolygonMode();
}

vk::CullModeFlagBits ToVkCullMode(CullMode mode)
{
    switch (mode)
    {
    case CullMode::FRONT:
        return vk::CullModeFlagBits::eFront;
    case CullMode::BACK:
        return vk::CullModeFlagBits::eBack;
    case CullMode::NONE:
        return vk::CullModeFlagBits::eNone;
    default:
        LOG_ERROR("ToVkCullMode: invalid enum!");
    }

    return vk::CullModeFlagBits::eNone;
}

vk::CompareOp ToVkCompareOp(ComparisonFunc func)
{
    switch (func)
    {
    case ComparisonFunc::NEVER:
        return vk::CompareOp::eNever;
    case ComparisonFunc::ALWAYS:
        return vk::CompareOp::eAlways;
    case ComparisonFunc::LESS:
        return vk::CompareOp::eLess;
    case ComparisonFunc::LESS_EQUAL:
        return vk::CompareOp::eLessOrEqual;
    case ComparisonFunc::GREATER:
        return vk::CompareOp::eGreater;
    case ComparisonFunc::GREATER_EQUAL:
        return vk::CompareOp::eGreaterOrEqual;
    case ComparisonFunc::NOT_EQUAL:
        return vk::CompareOp::eNotEqual;
    default:
        LOG_ERROR("ToVkCompareOp: invalid enum!");
    }

    return vk::CompareOp::eNever;
}

vk::StencilOp ToVkStencilOp(StencilOp stencilOp)
{
    switch (stencilOp)
    {
    case StencilOp::KEEP:
        return vk::StencilOp::eKeep;
    case StencilOp::ZERO:
        return vk::StencilOp::eZero;
    case StencilOp::REPLACE:
        return vk::StencilOp::eReplace;
    case StencilOp::INCR_SAT:
        return vk::StencilOp::eIncrementAndClamp;
    case StencilOp::DECR_SAT:
        return vk::StencilOp::eDecrementAndClamp;
    case StencilOp::INVERT:
        return vk::StencilOp::eInvert;
    case StencilOp::INCR:
        return vk::StencilOp::eIncrementAndWrap;
    case StencilOp::DECR:
        return vk::StencilOp::eDecrementAndWrap;
    default:
        LOG_ERROR("ToVkCompareOp: invalid enum!");
    }

    return vk::StencilOp::eKeep;
}

vk::StencilOpState ConvertStencilState(const DepthStencilState& depthStencilState, const StencilOpDesc& desc)
{
    return vk::StencilOpState()
        .setFailOp(ToVkStencilOp(desc.stencilFailOp))
        .setPassOp(ToVkStencilOp(desc.stencilPassOp))
        .setDepthFailOp(ToVkStencilOp(desc.stencilDepthFailOp))
        .setCompareOp(ToVkCompareOp(desc.stencilFunc))
        .setCompareMask(depthStencilState.stencilReadMask)
        .setWriteMask(depthStencilState.stencilWriteMask)
        .setReference(depthStencilState.stencilRefValue);
}

vk::Viewport ToVkViewport(Viewport vp)
{
    return vk::Viewport(vp.minX, vp.minY, vp.maxX - vp.minX, vp.maxY - vp.minY, vp.minZ, vp.maxZ);
}

} // namespace Vulkan

} // namespace SuohRHI