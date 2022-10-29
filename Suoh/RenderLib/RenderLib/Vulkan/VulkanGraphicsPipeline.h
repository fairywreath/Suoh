#pragma once

#include "VulkanCommon.h"

#include "VulkanImage.h"
#include "VulkanShader.h"

namespace RenderLib
{

namespace Vulkan
{

enum class RenderPassFlags : u8
{
    First = 0x01,
    Last = 0x02,
    Offscreen = 0x04,
    OffscreenInternal = 0x08,
};
ENUM_CLASS_FLAG_OPERATORS(RenderPassFlags);

struct RenderPassDesc
{
    bool clearColor{false};
    bool clearDepth{false};
    RenderPassFlags flags{0};

    bool hasDepth{true};
    vk::Format colorFormat{vk::Format::eB8G8R8A8Unorm};
};

class RenderPass : public RefCountResource<IResource>
{
public:
    explicit RenderPass(const VulkanContext& context) : m_Context(context)
    {
    }
    ~RenderPass();

    RenderPassDesc desc;
    vk::RenderPass renderPass{nullptr};

private:
    const VulkanContext& m_Context;
};

using RenderPassHandle = RefCountPtr<RenderPass>;

struct FramebufferDesc
{
    u32 width{0};
    u32 height{0};
    RenderPassHandle renderPass;
    std::vector<ImageHandle> attachments;
};

class Framebuffer : public RefCountResource<IResource>
{
public:
    explicit Framebuffer(const VulkanContext& context) : m_Context(context)
    {
    }
    ~Framebuffer();

    FramebufferDesc desc;
    vk::Framebuffer framebuffer{nullptr};

private:
    const VulkanContext& m_Context;
};

using FramebufferHandle = RefCountPtr<Framebuffer>;

struct GraphicsPipelineDesc
{
    // Pipline layout info.

    // Push constant ranges.
    u32 vertexConstSize{0};
    u32 fragmentConstSize{0};

    // XXX: replace with descriptor layout wrapper
    vk::DescriptorSetLayout descriptorSetLayout;

    // Graphics pipeline info.

    // Viewport and scissor info. Need to add data types for them?
    u32 width;
    u32 height;

    vk::PrimitiveTopology primitiveTopology{vk::PrimitiveTopology::eTriangleList};

    bool useDepth{true};
    bool useBlending{true};
    bool useDynamicScissorState{true};

    RenderPassHandle renderPass;
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    ShaderHandle geometryShader;

    // XXX: Raster info.
    // XXX: Multisample info.
    // XXX: Blend info.
};

class GraphicsPipeline : public RefCountResource<IResource>
{
public:
    explicit GraphicsPipeline(const VulkanContext& context) : m_Context(context)
    {
    }
    ~GraphicsPipeline();

    GraphicsPipelineDesc desc;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

private:
    const VulkanContext& m_Context;
};

using GraphicsPipelineHandle = RefCountPtr<GraphicsPipeline>;

} // namespace Vulkan

} // namespace RenderLib
