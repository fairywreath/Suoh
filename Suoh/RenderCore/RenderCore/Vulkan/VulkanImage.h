#pragma once

#include "VulkanCommon.h"

namespace RenderCore
{

namespace Vulkan
{

struct ImageDesc
{
    u32 width;
    u32 height;

    vk::Format format;
    vk::ImageTiling tiling{vk::ImageTiling::eOptimal};
    vk::ImageUsageFlags usage{0};
    vk::ImageCreateFlags flags{0};

    // Subresource view params.
    u32 mipLevels{1};
    u32 layerCount{1};
    vk::ImageViewType viewType{vk::ImageViewType::e2D};

    bool isColorAttachment{true};
    bool hasDepth{false};
    bool hasStencil{false};
};

class Image : public RefCountResource<IResource>
{
public:
    explicit Image(const VulkanContext& context) : m_Context(context)
    {
    }
    ~Image();

    // Create imageview after layout transitions.
    void CreateSubresourceView();

    ImageDesc desc;

    vk::Image image{nullptr};
    vk::ImageView imageView{nullptr};
    VmaAllocation allocation;

    vk::ImageLayout currentLayout{vk::ImageLayout::eUndefined};

    vk::Sampler sampler;

    bool managed{true};

private:
    const VulkanContext& m_Context;
};

using ImageHandle = RefCountPtr<Image>;

struct SamplerDesc
{
    vk::Filter minFilter{vk::Filter::eLinear};
    vk::Filter magFilter{vk::Filter::eLinear};
    vk::SamplerAddressMode addressMode{vk::SamplerAddressMode::eRepeat};
};

class Sampler : public RefCountResource<IResource>
{
public:
    explicit Sampler(const VulkanContext& context) : m_Context(context)
    {
    }
    ~Sampler();

    SamplerDesc desc;

    vk::Sampler sampler{nullptr};
    vk::SamplerCreateInfo samplerInfo;

private:
    const VulkanContext& m_Context;
};

using SamplerHandle = RefCountPtr<Sampler>;

} // namespace Vulkan

} // namespace RenderCore
