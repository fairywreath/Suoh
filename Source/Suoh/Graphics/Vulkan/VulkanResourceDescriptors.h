#pragma once

#include "VulkanCommon.h"
#include "VulkanState.h"

class Buffer;
class Sampler;
class Texture;
class ShaderState;
class DescriptorSetLayout;
class DescriptorSet;
class RenderPass;
class Framebuffer;
class Pipeline;

using GPUResourceRef = RefCountPtr<GPUResource>;
using BufferRef = RefCountPtr<Buffer>;
using SamplerRef = RefCountPtr<Sampler>;
using TextureRef = RefCountPtr<Texture>;
using DescriptorSetLayoutRef = RefCountPtr<DescriptorSetLayout>;
using DescriptorSetRef = RefCountPtr<DescriptorSet>;
using ShaderStateRef = RefCountPtr<ShaderState>;
using RenderPassRef = RefCountPtr<RenderPass>;
using FramebufferRef = RefCountPtr<Framebuffer>;
using PipelineRef = RefCountPtr<Pipeline>;

struct BufferDesc
{
    vk::BufferUsageFlags bufferUsage{0};
    ResourceUsageType resourceUsage{ResourceUsageType::Immutable};

    u32 size{0};

    bool persistent{false};
    bool deviceOnly{true};

    void* initialData{nullptr};

    std::string name;

    BufferDesc& Reset();
    BufferDesc& SetUsagFlags(vk::BufferUsageFlags _flags);
    BufferDesc& SetResourceUsage(ResourceUsageType _resourceUsage);
    BufferDesc& SetSize(u32 _size);
    BufferDesc& SetData(void* _data);
    BufferDesc& SetName(const std::string& _name);
    BufferDesc& SetPersistent(bool value);
    BufferDesc& SetDeviceOnly(bool value);
};

struct SamplerDesc
{
    vk::Filter minFilter{vk::Filter::eNearest};
    vk::Filter magFilter{vk::Filter::eNearest};
    vk::SamplerMipmapMode mipFilter{vk::SamplerMipmapMode::eNearest};

    vk::SamplerAddressMode addressModeU{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode addressModeV{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode addressModeW{vk::SamplerAddressMode::eRepeat};

    vk::SamplerReductionMode reductionMode{vk::SamplerReductionMode::eWeightedAverage};

    std ::string name;

    SamplerDesc& SetMinMagMip(vk::Filter min, vk::Filter mag, vk::SamplerMipmapMode mip);
    SamplerDesc& SetAddressModeU(vk::SamplerAddressMode u);
    SamplerDesc& SetAddressModeUV(vk::SamplerAddressMode u, vk::SamplerAddressMode v);
    SamplerDesc& SetAddressModeUVW(vk::SamplerAddressMode u, vk::SamplerAddressMode v, vk::SamplerAddressMode w);
    SamplerDesc& SetReductionMode(vk::SamplerReductionMode mode);
    SamplerDesc& SetName(const std::string& _name);
};

struct TextureDesc
{
    u32 width{1};
    u32 height{1};
    u32 depth{1};

    u32 arrayLayerCount{1};
    u32 mipLevelCount{1};

    void* initialData{nullptr};
    vk::Format format{vk::Format::eUndefined};

    TextureFlags flags;
    TextureType type{TextureType::Texture2D};

    // For aliased textures.
    TextureRef alias;

    std::string name;

    TextureDesc& SetSize(u32 _width, u32 _height, u32 _depth);
    TextureDesc& SetFlags(u32 _flags);
    TextureDesc& SetMips(u32 count);
    TextureDesc& SetLayers(u32 count);
    TextureDesc& SetFormatType(vk::Format _format, TextureType _type);
    TextureDesc& SetName(const std::string& _name);
    TextureDesc& SetData(void* _data);
    TextureDesc& SetAlias(const TextureRef& _alias);
};

struct TextureViewDesc
{
    Texture* parentTexture;

    u32 mipBaseLevel{0};
    u32 mipLevelCount{1};
    u32 arrayBaseLayer{0};
    u32 arrayLayerCount{1};

    std::string name;

    TextureViewDesc& SetParentTexture(TextureRef parent);
    TextureViewDesc& SetMips(u32 base, u32 count);
    TextureViewDesc& setArray(u32 base, u32 count);
    TextureViewDesc& SetName(const std::string& _name);
};

struct RenderPassColorTarget
{
    RenderPassColorTarget() = default;
    RenderPassColorTarget(vk::Format _format, vk::ImageLayout _layout, RenderPassOperation _operation)
        : format(_format), finalLayout(_layout), operation(_operation)
    {
    }

    vk::Format format{vk::Format::eUndefined};
    vk::ImageLayout finalLayout;
    RenderPassOperation operation;
};

struct RenderPassOutput
{
    std::vector<RenderPassColorTarget> colorTargets;

    vk::Format depthStencilFormat{vk::Format::eUndefined};
    vk::ImageLayout depthStencilFinalLayout;

    RenderPassOperation depthOperation{RenderPassOperation::DontCare};
    RenderPassOperation stencilOperation{RenderPassOperation::DontCare};

    std::string name;

    RenderPassOutput& Reset()
    {
        colorTargets.clear();
        depthStencilFormat = vk::Format::eUndefined;
        depthStencilFinalLayout = vk::ImageLayout::eUndefined;
        depthOperation = RenderPassOperation::DontCare;
        stencilOperation = RenderPassOperation::DontCare;
        return *this;
    }
    RenderPassOutput& AddColorTarget(vk::Format format, vk::ImageLayout layout, RenderPassOperation loadOp)
    {
        colorTargets.push_back(RenderPassColorTarget(format, layout, loadOp));
        return *this;
    }
    RenderPassOutput& SetDepthStencilTarget(vk::Format format, vk::ImageLayout layout)
    {
        depthStencilFormat = format;
        depthStencilFinalLayout = layout;
        return *this;
    }
    RenderPassOutput& SetDepthStencilOperations(RenderPassOperation _depthOp, RenderPassOperation _stencilOp)
    {
        depthOperation = _depthOp;
        stencilOperation = _stencilOp;
        return *this;
    }
    RenderPassOutput& SetName(const std::string& _name)
    {
        name = _name;
        return *this;
    }
};

struct RenderPassDesc
{
    std::vector<RenderPassColorTarget> colorTargets;

    vk::Format depthStencilFormat{vk::Format::eUndefined};
    vk::ImageLayout depthStencilFinalLayout;

    RenderPassOperation depthOperation{RenderPassOperation::DontCare};
    RenderPassOperation stencilOperation{RenderPassOperation::DontCare};

    std::string name;

    RenderPassDesc& Reset()
    {
        colorTargets.clear();
        depthStencilFormat = vk::Format::eUndefined;
        depthStencilFinalLayout = vk::ImageLayout::eUndefined;
        depthOperation = RenderPassOperation::DontCare;
        stencilOperation = RenderPassOperation::DontCare;
        name = "";
        return *this;
    }
    RenderPassDesc& AddColorTarget(vk::Format format, vk::ImageLayout layout, RenderPassOperation loadOp)
    {
        colorTargets.push_back(RenderPassColorTarget(format, layout, loadOp));
        return *this;
    }
    RenderPassDesc& SetDepthTarget(vk::Format format, vk::ImageLayout layout)
    {
        depthStencilFormat = format;
        depthStencilFinalLayout = layout;
        return *this;
    }
    RenderPassDesc& SetDepthStencilOperations(RenderPassOperation depthOp, RenderPassOperation stencilOp)
    {
        depthOperation = depthOp;
        stencilOperation = stencilOp;
        return *this;
    }
    RenderPassDesc& SetName(const std::string& _name)
    {
        name = _name;
        return *this;
    }
};

struct FramebufferDesc
{
    RenderPass* renderPass{nullptr};
    std::vector<Texture*> colorAttachments;
    Texture* depthStencilAttachment{nullptr};

    u32 width{0};
    u32 height{0};

    f32 scaleX{1.0f};
    f32 scaleY{1.0f};
    bool resize{false};

    std::string name;
}

struct PipelineDesc
{
    VertexInputDesc vertexInput{};
    BlendStatesDesc blendState{};
    RasterizationState rasterizationState{};
    DepthStencilState depthStencilState{};

    u32 vertexConstSize{0};
    u32 fragmentConstSize{0};

    // XXX: Ideally we wont need these(and have creation descriptors of them instead), in case dynamic rendering is
    // used.
    ShaderState* shaderState;
    RenderPass* renderPass;

    std::vector<DescriptorSetLayout*> descriptorSetLayouts;

    // RenderPassOutput renderPassOutput;

    vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
    const ViewportState* viewport{nullptr};

    // Viewport state dimensions.
    u32 width{1};
    u32 height{1};

    // XXX: Params for dynamic states?

    std::string name;
    std::string pipelineCachePath;

    PipelineType type;
};
