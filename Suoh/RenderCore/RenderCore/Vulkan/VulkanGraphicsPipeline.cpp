#include "VulkanGraphicsPipeline.h"

#include "VulkanDevice.h"

namespace RenderCore
{

namespace Vulkan
{

RenderPass::~RenderPass()
{
    if (renderPass)
    {
        m_Context.device.destroyRenderPass(renderPass);
    }
}

Framebuffer::~Framebuffer()
{
    if (framebuffer)
    {
        m_Context.device.destroyFramebuffer(framebuffer);
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    if (pipeline)
    {
        m_Context.device.destroyPipeline(pipeline);
    }
    if (pipelineLayout)
    {
        m_Context.device.destroyPipelineLayout(pipelineLayout);
    }
}

GraphicsPipelineHandle VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    auto handle = GraphicsPipelineHandle::Create(new GraphicsPipeline(m_Context));

    std::vector<vk::PushConstantRange> pushConstantRanges;
    if (desc.vertexConstSize > 0)
    {
        pushConstantRanges.push_back(vk::PushConstantRange()
                                         .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                                         .setOffset(0)
                                         .setSize(desc.vertexConstSize));
    }
    if (desc.fragmentConstSize > 0)
    {
        pushConstantRanges.push_back(vk::PushConstantRange()
                                         .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                                         .setOffset(0)
                                         .setSize(desc.fragmentConstSize));
    }

    // Create pipeline layout.
    const auto layoutInfo = vk::PipelineLayoutCreateInfo()
                                .setSetLayoutCount(1)
                                .setPSetLayouts(&desc.descriptorSetLayout)
                                .setPushConstantRangeCount(pushConstantRanges.size())
                                .setPPushConstantRanges(pushConstantRanges.data());

    VK_CHECK_RETURN_NULL(
        m_Context.device.createPipelineLayout(&layoutInfo, nullptr, &handle->pipelineLayout));

    // Configure shader stages.
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    const auto AddShaderStage = [&](vk::ShaderModule module, vk::ShaderStageFlagBits stage) {
        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo().setStage(stage).setModule(module).setPName("main"));
    };

    if (desc.vertexShader != nullptr)
    {
        AddShaderStage(desc.vertexShader->shaderModule, vk::ShaderStageFlagBits::eVertex);
    }
    if (desc.fragmentShader != nullptr)
    {
        AddShaderStage(desc.vertexShader->shaderModule, vk::ShaderStageFlagBits::eFragment);
    }
    if (desc.geometryShader != nullptr)
    {
        AddShaderStage(desc.vertexShader->shaderModule, vk::ShaderStageFlagBits::eGeometry);
    }

    if (shaderStages.empty())
    {
        LOG_ERROR("CreateGraphicsPipeline: no shader passed in!");
        return nullptr;
    }

    const auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo();

    const auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo()
                                   .setTopology(desc.primitiveTopology)
                                   .setPrimitiveRestartEnable(false);

    const auto viewport = vk::Viewport()
                              .setX(0.0f)
                              .setY(0.0f)
                              .setWidth((float)desc.width)
                              .setHeight((float)desc.height)
                              .setMinDepth(0.0f)
                              .setMaxDepth(0.0f);

    const auto scissor = vk::Rect2D().setOffset({0, 0}).setExtent({desc.width, desc.height});

    const auto viewportState = vk::PipelineViewportStateCreateInfo()
                                   .setViewportCount(1)
                                   .setViewports(viewport)
                                   .setScissorCount(1)
                                   .setScissors(scissor);

    const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo()
                                        .setPolygonMode(vk::PolygonMode::eFill)
                                        .setCullMode(vk::CullModeFlagBits::eNone)
                                        .setFrontFace(vk::FrontFace::eClockwise)
                                        .setLineWidth(1.0f);

    const auto multisampleState = vk::PipelineMultisampleStateCreateInfo()
                                      .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                                      .setSampleShadingEnable(false)
                                      .setMinSampleShading(1.0f);

    const auto colorBlendAttachment
        = vk::PipelineColorBlendAttachmentState()
              .setBlendEnable(true)
              .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
              .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
              .setColorBlendOp(vk::BlendOp::eAdd)
              .setSrcAlphaBlendFactor(desc.useBlending ? vk::BlendFactor::eOneMinusSrcAlpha
                                                       : vk::BlendFactor::eOne)
              .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
              .setAlphaBlendOp(vk::BlendOp::eAdd)
              .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                 | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo()
                                     .setLogicOpEnable(false)
                                     .setLogicOp(vk::LogicOp::eCopy)
                                     .setAttachmentCount(1)
                                     .setAttachments(colorBlendAttachment)
                                     .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});

    const auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo()
                                       .setDepthTestEnable(desc.useDepth)
                                       .setDepthWriteEnable(desc.useDepth)
                                       .setDepthCompareOp(vk::CompareOp::eLess)
                                       .setDepthBoundsTestEnable(false)
                                       .setMinDepthBounds(0.0f)
                                       .setMaxDepthBounds(0.0f);

    std::vector<vk::DynamicState> dynamicStates;
    if (desc.useDynamicScissorState)
        dynamicStates.push_back(vk::DynamicState::eScissor);

    const auto dynamicState = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

    const auto pipelineInfo
        = vk::GraphicsPipelineCreateInfo()
              .setStages(shaderStages)
              .setPVertexInputState(&vertexInputInfo)
              .setPInputAssemblyState(&inputAssembly)
              .setPTessellationState(nullptr)
              .setPViewportState(&viewportState)
              .setPRasterizationState(&rasterizationState)
              .setPMultisampleState(&multisampleState)
              .setPDepthStencilState(desc.useDepth ? &depthStencilState : nullptr)
              .setPColorBlendState(&colorBlendState)
              .setPDynamicState(desc.useDynamicScissorState ? &dynamicState : nullptr)
              .setLayout(handle->pipelineLayout)
              .setRenderPass(desc.renderPass->renderPass)
              .setSubpass(0)
              .setBasePipelineHandle(nullptr)
              .setBasePipelineIndex(-1);

    VK_CHECK_RETURN_NULL(m_Context.device.createGraphicsPipelines(nullptr, 1, &pipelineInfo,
                                                                  nullptr, &handle->pipeline));

    return handle;
}

} // namespace Vulkan

} // namespace RenderCore
