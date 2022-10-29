#include "VulkanGraphicsPipeline.h"

#include "VulkanDevice.h"

namespace RenderLib
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

RenderPassHandle VulkanDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    const bool first = (desc.flags & RenderPassFlags::First) != 0;
    const bool last = (desc.flags & RenderPassFlags::Last) != 0;
    const bool offscreenInternal = (desc.flags & RenderPassFlags::OffscreenInternal) != 0;
    const bool offscreen = (desc.flags & RenderPassFlags::Offscreen) != 0;

    auto colorAttachment
        = vk::AttachmentDescription()
              .setFormat(desc.colorFormat)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setLoadOp(offscreenInternal ? vk::AttachmentLoadOp::eLoad
                                           : (desc.clearColor ? vk::AttachmentLoadOp::eClear
                                                              : vk::AttachmentLoadOp::eDontCare))
              .setStoreOp(vk::AttachmentStoreOp::eDontCare)
              .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
              .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
              .setInitialLayout(first ? vk::ImageLayout::eUndefined
                                      : (offscreenInternal
                                             ? vk::ImageLayout::eShaderReadOnlyOptimal
                                             : vk::ImageLayout::eColorAttachmentOptimal))
              .setFinalLayout(last ? vk::ImageLayout::ePresentSrcKHR
                                   : vk::ImageLayout::eColorAttachmentOptimal);

    auto colorAttachmentRef = vk::AttachmentReference().setAttachment(0).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal);

    auto depthAttachment
        = vk::AttachmentDescription()
              .setFormat(desc.hasDepth ? FindDepthFormat() : vk::Format::eD32Sfloat)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setLoadOp(offscreenInternal ? vk::AttachmentLoadOp::eLoad
                                           : (desc.clearDepth ? vk::AttachmentLoadOp::eClear
                                                              : vk::AttachmentLoadOp::eDontCare))
              .setStoreOp(vk::AttachmentStoreOp::eDontCare)
              .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
              .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
              .setInitialLayout(first ? vk::ImageLayout::eUndefined
                                      : (offscreenInternal
                                             ? vk::ImageLayout::eShaderReadOnlyOptimal
                                             : vk::ImageLayout::eDepthAttachmentOptimal))
              .setFinalLayout(vk::ImageLayout::eDepthAttachmentOptimal);

    auto depthAttachmentRef = vk::AttachmentReference().setAttachment(1).setLayout(
        vk::ImageLayout::eDepthAttachmentOptimal);

    std::vector<vk::SubpassDependency> subpassDependencies
        = {vk::SubpassDependency()
               .setSrcSubpass(VK_SUBPASS_EXTERNAL)
               .setDstSubpass(0)
               .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
               .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
               .setSrcAccessMask(vk::AccessFlagBits(0))
               .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead
                                 | vk::AccessFlagBits::eColorAttachmentWrite)

        };

    if (offscreen)
    {
        colorAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        depthAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        // Setup subpass for layout transitions.
        subpassDependencies.resize(2);
        subpassDependencies[0]
            = vk::SubpassDependency()
                  .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                  .setDstSubpass(0)
                  .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                  .setSrcAccessMask(vk::AccessFlagBits(0))
                  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                  .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
        subpassDependencies[1]
            = vk::SubpassDependency()
                  .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                  .setDstSubpass(0)
                  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                  .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                  .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                  .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                  .setDependencyFlags(vk::DependencyFlagBits::eByRegion);
    }

    auto subpass = vk::SubpassDescription()
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setInputAttachmentCount(0)
                       .setPInputAttachments(nullptr)
                       .setColorAttachmentCount(1)
                       .setPColorAttachments(&colorAttachmentRef)
                       .setPDepthStencilAttachment(desc.hasDepth ? &depthAttachmentRef : nullptr)
                       .setPreserveAttachmentCount(0)
                       .setPPreserveAttachments(nullptr);

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    auto renderPassInfo = vk::RenderPassCreateInfo()
                              .setAttachmentCount(desc.hasDepth ? 2 : 1)
                              .setPAttachments(attachments.data())
                              .setSubpassCount(1)
                              .setPSubpasses(&subpass)
                              .setDependencyCount((u32)subpassDependencies.size())
                              .setPDependencies(subpassDependencies.data());

    auto renderPass = RenderPassHandle::Create(new RenderPass(m_Context));
    VK_CHECK_RETURN_NULL(
        m_Context.device.createRenderPass(&renderPassInfo, nullptr, &renderPass->renderPass));

    return renderPass;
}

FramebufferHandle VulkanDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    auto framebuffer = FramebufferHandle::Create(new Framebuffer(m_Context));

    std::vector<vk::ImageView> attachments;
    attachments.reserve(desc.attachments.size());

    for (const auto& image : desc.attachments)
    {
        attachments.push_back(image->imageView);
    }

    const auto framebufferInfo = vk::FramebufferCreateInfo()
                                     .setRenderPass(desc.renderPass->renderPass)
                                     .setAttachmentCount((u32)attachments.size())
                                     .setPAttachments(attachments.data())
                                     .setWidth(desc.width)
                                     .setHeight(desc.height)
                                     .setLayers(1);

    VK_CHECK_RETURN_NULL(
        m_Context.device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer->framebuffer));

    return framebuffer;
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
        AddShaderStage(desc.fragmentShader->shaderModule, vk::ShaderStageFlagBits::eFragment);
    }
    if (desc.geometryShader != nullptr)
    {
        AddShaderStage(desc.geometryShader->shaderModule, vk::ShaderStageFlagBits::eGeometry);
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

} // namespace RenderLib
