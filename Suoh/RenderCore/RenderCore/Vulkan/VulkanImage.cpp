#include "VulkanImage.h"

namespace RenderCore
{

namespace Vulkan
{

Image::~Image()
{
    if (managed)
    {
        if (sampler)
            m_Context.device.destroySampler(sampler);
        if (imageView)
            m_Context.device.destroyImageView(imageView);
        if (image)
            m_Context.device.destroyImage(image);
    }
}

void Image::CreateSubresourceView()
{
    if (imageView)
    {
        m_Context.device.destroyImageView(imageView);
        imageView = nullptr;
    }

    // Create image view.
    auto aspectFlags = vk::ImageAspectFlags();
    if (desc.isColorAttachment)
    {
        aspectFlags |= vk::ImageAspectFlagBits::eColor;
        assert(!desc.hasDepth && !desc.hasStencil);
    }
    else
    {
        if (desc.hasDepth)
            aspectFlags |= vk::ImageAspectFlagBits::eDepth;
        if (desc.hasStencil)
            aspectFlags |= vk::ImageAspectFlagBits::eStencil;
    }

    auto viewInfo = vk::ImageViewCreateInfo()
                        .setFlags(vk::ImageViewCreateFlags(0))
                        .setImage(image)
                        .setViewType(desc.viewType)
                        .setFormat(desc.format)
                        .setSubresourceRange(vk::ImageSubresourceRange()
                                                 .setAspectMask(aspectFlags)
                                                 .setBaseMipLevel(0)
                                                 .setLevelCount(desc.mipLevels)
                                                 .setBaseArrayLayer(0)
                                                 .setLayerCount(desc.layerCount));

    VK_CHECK_RETURN(m_Context.device.createImageView(&viewInfo, nullptr, &imageView));
}

Sampler::~Sampler()
{
    if (sampler)
        m_Context.device.destroySampler(sampler);
}

} // namespace Vulkan

} // namespace RenderCore
