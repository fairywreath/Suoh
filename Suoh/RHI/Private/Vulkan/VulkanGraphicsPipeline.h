#pragma once

#include "VulkanCommon.h"

#include "VulkanBindings.h"
#include "VulkanShader.h"

#include <vector>

namespace SuohRHI
{

namespace Vulkan
{

class VulkanFramebuffer : public RefCountResource<IFramebuffer>
{
public:
    explicit VulkanFramebuffer(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanFramebuffer() override;

    const FramebufferDesc& GetDesc() const override
    {
        return desc;
    }
    const FramebufferInfo& GetFramebufferInfo() const override
    {
        return framebufferInfo;
    }
    Object GetNativeObject(ObjectType type) override;

    FramebufferDesc desc;
    FramebufferInfo framebufferInfo;

    vk::RenderPass renderPass{};
    vk::Framebuffer framebuffer{};
    bool managed{true};

    std::vector<ResourceHandle> resources;

private:
    const VulkanContext& m_Context;
};

class VulkanGraphicsPipeline : public RefCountResource<IGraphicsPipeline>
{
public:
    explicit VulkanGraphicsPipeline(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanGraphicsPipeline() override;

    Object GetNativeObject(ObjectType objectType) override;
    const GraphicsPipelineDesc& GetDesc() const override
    {
        return desc;
    }
    const FramebufferInfo& GetFramebufferInfo() const override
    {

        return framebufferInfo;
    }

    GraphicsPipelineDesc desc;
    FramebufferInfo framebufferInfo;
    ShaderType shaderMask{ShaderType::NONE};

    BindingVector<RefCountPtr<VulkanBindingLayout>> pipelineBindingLayouts;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    bool useBlendConstats{false};

private:
    const VulkanContext& m_Context;
};

} // namespace Vulkan

} // namespace SuohRHI