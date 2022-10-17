#include "RHI.h"

namespace SuohRHI
{

TextureLayer TextureLayer::Resolve(const TextureDesc& desc) const
{
    TextureLayer ret(*this);

    assert(mipLevel < desc.mipLevels);

    if (width == u32(-1))
        ret.width = (desc.width >> mipLevel);

    if (height == u32(-1))
        ret.height = (desc.height >> mipLevel);

    if (depth == u32(-1))
    {
        if (desc.dimension == TextureDimension::TEXTURE_3D)
            ret.depth = (desc.depth >> mipLevel);
        else
            ret.depth = 1;
    }

    return ret;
}

TextureSubresourceSet TextureSubresourceSet::Resolve(const TextureDesc& desc, bool singleMipLevel) const
{
    TextureSubresourceSet ret;
    ret.baseMipLevel = baseMipLevel;

    if (singleMipLevel)
    {
        ret.numMipLevels = 1;
    }
    else
    {
        int lastMipLevelPlusOne = std::min(baseMipLevel + numMipLevels, desc.mipLevels);
        ret.numMipLevels = MipLevel(std::max(0u, lastMipLevelPlusOne - baseMipLevel));
    }

    switch (desc.dimension) // NOLINT(clang-diagnostic-switch-enum)
    {
    case TextureDimension::TEXTURE_1D_ARRAY:
    case TextureDimension::TEXTURE_2D_ARRAY:
    case TextureDimension::TEXTURE_CUBE:
    case TextureDimension::TEXTURE_CUBE_ARRAY:
    case TextureDimension::TEXTURE_2DMS_ARRAY: {
        ret.baseArrayLayer = baseArrayLayer;
        int lastArraySlicePlusOne = std::min(baseArrayLayer + numArrayLayers, desc.layerCount);
        ret.numArrayLayers = ArrayLayer(std::max(0u, lastArraySlicePlusOne - baseArrayLayer));
        break;
    }
    default:
        ret.baseArrayLayer = 0;
        ret.numArrayLayers = 1;
        break;
    }

    return ret;
}

bool TextureSubresourceSet::IsEntireTexture(const TextureDesc& desc) const
{
    if (baseMipLevel > 0u || baseMipLevel + numMipLevels < desc.mipLevels)
        return false;

    switch (desc.dimension) // NOLINT(clang-diagnostic-switch-enum)
    {
    case TextureDimension::TEXTURE_1D_ARRAY:
    case TextureDimension::TEXTURE_2D_ARRAY:
    case TextureDimension::TEXTURE_CUBE:
    case TextureDimension::TEXTURE_CUBE_ARRAY:
    case TextureDimension::TEXTURE_2DMS_ARRAY:
        if (baseArrayLayer > 0u || (baseArrayLayer + numArrayLayers) < desc.layerCount)
            return false;
    default:
        return true;
    }
}

BufferRange BufferRange::Resolve(const BufferDesc& desc) const
{
    BufferRange result;
    result.byteOffset = std::min(byteOffset, desc.byteSize);
    if (byteSize == 0)
        result.byteSize = desc.byteSize - result.byteOffset;
    else
        result.byteSize = std::min(byteSize, desc.byteSize - result.byteOffset);
    return result;
}

// bool RTBlendState::UsesConstantColor() const
//{
//     return srcBlend == BlendFactor::ConstantColor || srcBlend == BlendFactor::OneMinusConstantColor
//            || destBlend == BlendFactor::ConstantColor || destBlend == BlendFactor::OneMinusConstantColor
//            || srcBlendAlpha == BlendFactor::ConstantColor || srcBlendAlpha == BlendFactor::OneMinusConstantColor
//            || destBlendAlpha == BlendFactor::ConstantColor || destBlendAlpha == BlendFactor::OneMinusConstantColor;
// }

// bool BlendState::usesConstantColor(uint32_t numTargets) const
//{
//     for (uint32_t rt = 0; rt < numTargets; rt++)
//     {
//         if (targets[rt].usesConstantColor())
//             return true;
//     }
//
//     return false;
// }

FramebufferInfo::FramebufferInfo(const FramebufferDesc& desc)
{
    for (size_t i = 0; i < desc.colorAttachments.size(); i++)
    {
        const FramebufferAttachment& attachment = desc.colorAttachments[i];
        colorFormats.push_back(attachment.format == Format::UNKNOWN && attachment.texture ? attachment.texture->GetDesc().format
                                                                                          : attachment.format);
    }

    if (desc.depthAttachment.Valid())
    {
        const TextureDesc& textureDesc = desc.depthAttachment.texture->GetDesc();
        depthFormat = textureDesc.format;
        width = textureDesc.width >> desc.depthAttachment.subresources.baseMipLevel;
        height = textureDesc.height >> desc.depthAttachment.subresources.baseMipLevel;
        sampleCount = textureDesc.sampleCount;
        sampleQuality = textureDesc.sampleQuality;
    }
    else if (!desc.colorAttachments.empty() && desc.colorAttachments[0].Valid())
    {
        const TextureDesc& textureDesc = desc.colorAttachments[0].texture->GetDesc();
        width = textureDesc.width >> desc.colorAttachments[0].subresources.baseMipLevel;
        height = textureDesc.height >> desc.colorAttachments[0].subresources.baseMipLevel;
        sampleCount = textureDesc.sampleCount;
        sampleQuality = textureDesc.sampleQuality;
    }
}

void ICommandList::SetResourceStatesForFramebuffer(IFramebuffer* framebuffer)
{
    const FramebufferDesc& desc = framebuffer->GetDesc();

    for (const auto& attachment : desc.colorAttachments)
    {
        SetTextureState(attachment.texture, attachment.subresources, ResourceStates::RENDER_TARGET);
    }

    if (desc.depthAttachment.Valid())
    {
        SetTextureState(desc.depthAttachment.texture, desc.depthAttachment.subresources,
                        desc.depthAttachment.isReadOnly ? ResourceStates::DEPTH_READ : ResourceStates::DEPTH_WRITE);
    }
}

} // namespace SuohRHI