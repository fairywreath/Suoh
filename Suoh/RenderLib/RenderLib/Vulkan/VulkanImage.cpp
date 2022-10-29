#include "VulkanImage.h"

#include "VulkanDevice.h"

namespace RenderLib
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

ImageHandle VulkanDevice::CreateImage(const ImageDesc& desc)
{
    auto image = ImageHandle::Create(new Image(m_Context));

    auto usage = desc.usage | vk::ImageUsageFlagBits::eTransferDst;

    // Create image.
    auto imageInfo
        = vk::ImageCreateInfo()
              .setFlags(desc.flags)
              .setImageType(vk::ImageType::e2D)
              .setFormat(desc.format)
              .setExtent(vk::Extent3D().setWidth(desc.width).setHeight(desc.height).setDepth(1))
              .setMipLevels(1)
              .setArrayLayers((desc.flags & vk::ImageCreateFlagBits::eCubeCompatible) ? 6 : 1)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setTiling(desc.tiling)
              .setUsage(usage)
              .setSharingMode(vk::SharingMode::eExclusive)
              .setInitialLayout(vk::ImageLayout::eUndefined);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    auto res = vmaCreateImage(m_Context.allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo),
                              &allocInfo, reinterpret_cast<VkImage*>(&image->image),
                              &image->allocation, nullptr);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("vmaCreateImage failed!");
        return nullptr;
    }

    image->desc = desc;
    image->desc.usage = usage;
    image->currentLayout = vk::ImageLayout::eUndefined;

    return image;
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    auto sampler = SamplerHandle::Create(new Sampler(m_Context));

    sampler->samplerInfo = vk::SamplerCreateInfo()
                               .setMagFilter(desc.magFilter)
                               .setMinFilter(desc.minFilter)
                               .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                               .setAddressModeU(desc.addressMode)
                               .setAddressModeV(desc.addressMode)
                               .setAddressModeW(desc.addressMode)
                               .setMipLodBias(1.0f)
                               .setAnisotropyEnable(false)
                               .setMaxAnisotropy(1)
                               .setCompareEnable(false)
                               .setCompareOp(vk::CompareOp::eAlways)
                               .setMinLod(0.0f)
                               .setMaxLod(0.0f)
                               .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                               .setUnnormalizedCoordinates(false);

    VK_CHECK_RETURN_NULL(
        m_Context.device.createSampler(&sampler->samplerInfo, nullptr, &sampler->sampler));

    return sampler;
}

} // namespace Vulkan

} // namespace RenderLib
