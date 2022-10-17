#include "VulkanGraphicsPipeline.h"

namespace SuohRHI
{

namespace Vulkan
{

namespace
{

} // namespace

VulkanFramebuffer::~VulkanFramebuffer()
{
    if (renderPass && managed)
    {
        m_Context.device.destroyRenderPass(renderPass);
        renderPass = nullptr;
    }

    if (framebuffer && managed)
    {
        m_Context.device.destroyFramebuffer(framebuffer);
        framebuffer = nullptr;
    }
}

Object VulkanFramebuffer::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_RenderPass:
        return Object(renderPass);
    case ObjectTypes::Vk_FrameBuffer:
        return Object(framebuffer);
    default:
        return Object(nullptr);
    }
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (pipelineLayout)
    {
        m_Context.device.destroyPipeline(pipeline);
        pipeline = nullptr;
    }

    if (pipeline)
    {
        m_Context.device.destroyPipelineLayout(pipelineLayout);
        pipelineLayout = nullptr;
    }
}

Object VulkanGraphicsPipeline::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_Pipeline:
        return Object(pipeline);
    case ObjectTypes::Vk_PipelineLayout:
        return Object(pipelineLayout);
    default:
        return Object(nullptr);
    }
}

} // namespace Vulkan

} // namespace SuohRHI