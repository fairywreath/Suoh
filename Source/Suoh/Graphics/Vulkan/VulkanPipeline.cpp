#include "VulkanDevice.h"

#include <filesystem>

namespace fs = std::filesystem;

RenderPassOutput VulkanDevice::FillRenderPassOutput(const RenderPassDesc& desc) const
{
    RenderPassOutput output;
    output.Reset();

    for (const auto& colorTarget : desc.colorTargets)
    {
        output.AddColorTarget(colorTarget.format, colorTarget.finalLayout, colorTarget.operation);
    }

    if (desc.depthStencilFormat != vk::Format::eUndefined)
    {
        output.SetDepthStencilTarget(desc.depthStencilFormat, desc.depthStencilFinalLayout);
        output.SetDepthStencilOperations(desc.depthOperation, desc.stencilOperation);
    }

    return output;
}

RenderPass::RenderPass(VulkanDevice* device, const RenderPassDesc& desc) : m_Device(std::move(device))
{
    m_NumColorRenderTargets = desc.colorTargets.size();

    m_Name = desc.name;
    m_Output = m_Device->FillRenderPassOutput(desc);

    vk::AttachmentLoadOp depthOp;
    vk::AttachmentLoadOp stencilOp;
    vk::ImageLayout depthInitialImageLayout;

    bool renderDepth = (m_Output.depthStencilFormat != vk::Format::eUndefined);

    switch (m_Output.depthOperation)
    {
    case RenderPassOperation::Load:
        depthOp = vk::AttachmentLoadOp::eLoad;
        depthInitialImageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        break;
    case RenderPassOperation::Clear:
        depthOp = vk::AttachmentLoadOp::eClear;
        depthInitialImageLayout = vk::ImageLayout::eUndefined;
        break;
    default:
        depthOp = vk::AttachmentLoadOp::eDontCare;
        depthInitialImageLayout = vk::ImageLayout::eUndefined;
        break;
    }

    switch (m_Output.stencilOperation)
    {
    case RenderPassOperation::Load:
        stencilOp = vk::AttachmentLoadOp::eLoad;
        break;
    case RenderPassOperation::Clear:
        stencilOp = vk::AttachmentLoadOp::eClear;
        break;
    default:
        stencilOp = vk::AttachmentLoadOp::eDontCare;
        break;
    }

    // Color attachments.
    std::array<vk::AttachmentDescription, GPUConstants::MaxRenderTargets> colorTargets;
    std::array<vk::AttachmentReference, GPUConstants::MaxRenderTargets> colorAttachmentRefs;

    u32 currentAttachmentIndex = 0;

    for (; currentAttachmentIndex < m_Output.colorTargets.size(); currentAttachmentIndex++)
    {
        vk::AttachmentLoadOp colorOp;
        vk::ImageLayout colorInitialImageLayout;

        switch (m_Output.colorTargets[currentAttachmentIndex].operation)
        {
        case RenderPassOperation::Load:
            colorOp = vk::AttachmentLoadOp::eLoad;
            colorInitialImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            break;
        case RenderPassOperation::Clear:
            colorOp = vk::AttachmentLoadOp::eClear;
            colorInitialImageLayout = vk::ImageLayout::eUndefined;
            break;
        default:
            colorOp = vk::AttachmentLoadOp::eDontCare;
            colorInitialImageLayout = vk::ImageLayout::eUndefined;
            break;
        }

        auto colorAttachment = vk::AttachmentDescription()
                                   .setFormat(m_Output.colorTargets[currentAttachmentIndex].format)
                                   .setSamples(vk::SampleCountFlagBits::e1)
                                   .setLoadOp(colorOp)
                                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                                   .setStencilLoadOp(stencilOp)
                                   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                   .setInitialLayout(colorInitialImageLayout)
                                   .setFinalLayout(m_Output.colorTargets[currentAttachmentIndex].finalLayout);

        auto colorAttachmentRef = vk::AttachmentReference()
                                      .setAttachment(currentAttachmentIndex)
                                      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        colorTargets[currentAttachmentIndex] = std::move(colorAttachment);
        colorAttachmentRefs[currentAttachmentIndex] = std::move(colorAttachmentRef);
    }

    // Depth attachment.
    vk::AttachmentDescription depthAttachment{};
    vk::AttachmentReference depthAttachmentRef{};

    if (renderDepth)
    {
        depthAttachment.setFormat(m_Output.depthStencilFormat)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(depthOp)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(stencilOp)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(depthInitialImageLayout)
            .setFinalLayout(m_Output.depthStencilFinalLayout);

        depthAttachmentRef.setAttachment(currentAttachmentIndex)
            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    }

    // Subpass.
    std::array<vk::AttachmentDescription, GPUConstants::MaxRenderTargets + 1> activeAttachments;
    u32 depthStencilCount = 0;
    for (u32 i = 0; i < m_Output.colorTargets.size(); i++)
    {
        activeAttachments[i] = colorTargets[i];
    }
    if (renderDepth)
    {
        activeAttachments[m_Output.colorTargets.size()] = depthAttachment;
        depthStencilCount = 1;
    }

    auto subpassDescription = vk::SubpassDescription()
                                  .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                                  .setInputAttachmentCount(0)
                                  .setPInputAttachments(nullptr)
                                  .setColorAttachmentCount(m_Output.colorTargets.size())
                                  .setPColorAttachments(colorAttachmentRefs.data())
                                  .setPDepthStencilAttachment(renderDepth ? &depthAttachmentRef : nullptr)
                                  .setPreserveAttachmentCount(0)
                                  .setPPreserveAttachments(nullptr);

    // XXX: Subpass dependencies?

    auto renderPassInfo = vk::RenderPassCreateInfo()
                              .setAttachmentCount(m_Output.colorTargets.size() + depthStencilCount)
                              .setPAttachments(activeAttachments.data())
                              .setSubpasses(subpassDescription);

    auto vkRes = m_Device->GetVulkanDevice().createRenderPass(&renderPassInfo, m_Device->GetVulkanAllocationCallbacks(),
                                                              &m_VulkanRenderPass);
    VK_CHECK(vkRes);

    m_Device->SetVulkanResourceName(vk::ObjectType::eRenderPass, m_VulkanRenderPass, m_Name.c_str());

    m_Desc = desc;
}

RenderPass::~RenderPass()
{
    if (m_VulkanRenderPass)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::RenderPass,
                                                             m_VulkanRenderPass);
    }
}

RenderPassRef VulkanDevice::CreateRenderPass(const RenderPassDesc& desc)
{
    auto renderPassRef = RenderPassRef::Create(new RenderPass(this, desc));
    if (!renderPassRef->GetVulkanHandle())
    {
        renderPassRef = nullptr;
    }
    return nullptr;
}

Framebuffer::Framebuffer(VulkanDevice* device, const FramebufferDesc& desc) : m_Device(std::move(device))
{
    const bool renderDepth = (desc.depthStencilAttachment != nullptr);
    const u32 numAttachments = (renderDepth) ? (desc.colorAttachments.size() + 1) : desc.colorAttachments.size();
    std::vector<vk::ImageView> attachments;
    attachments.reserve(numAttachments);

    for (const auto texture : desc.colorAttachments)
    {
        assert(texture->GetVulkanView());
        attachments.push_back(texture->GetVulkanView());
    }

    if (renderDepth)
    {
        assert(desc.depthStencilAttachment->GetVulkanView());
        attachments.push_back(desc.depthStencilAttachment->GetVulkanView());
    }

    const auto framebufferInfo = vk::FramebufferCreateInfo()
                                     .setRenderPass(desc.renderPass->GetVulkanHandle())
                                     .setAttachments(attachments)
                                     .setWidth(desc.width)
                                     .setHeight(desc.height)
                                     .setLayers(1);
    auto vkRes = m_Device->GetVulkanDevice().createFramebuffer(
        &framebufferInfo, m_Device->GetVulkanAllocationCallbacks(), &m_VulkanFramebuffer);
    VK_CHECK(vkRes);
    if (vkRes != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create vulkan framebuffer!");
        return;
    }

    m_Device->SetVulkanResourceName(vk::ObjectType::eFramebuffer, m_VulkanFramebuffer, desc.name);

    // Framebuffer is sucessfully created, copy info from desc.
    m_Width = desc.width;
    m_Height = desc.height;
    m_Name = desc.name;
    m_Resize = desc.resize;
    m_ScaleX = desc.scaleX;
    m_ScaleY = desc.scaleY;

    // Create strong reference for GPU resources to be used.
    m_RenderPass = RenderPassRef::Create(desc.renderPass);
    m_DepthStencilAttachment = TextureRef::Create(desc.depthStencilAttachment);
    for (auto colorTexture : desc.colorAttachments)
    {
        m_ColorAttachments.push_back(TextureRef::Create(colorTexture));
    }

    m_Desc = desc;
}

Framebuffer::~Framebuffer()
{
    if (m_VulkanFramebuffer)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::Framebuffer,
                                                             m_VulkanFramebuffer);
    }
}

FramebufferRef VulkanDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    auto framebufferRef = FramebufferRef::Create(new Framebuffer(this, desc));
    if (!framebufferRef->GetVulkanHandle())
    {
        framebufferRef = nullptr;
    }
    return framebufferRef;
}

Pipeline::Pipeline(VulkanDevice* device, const PipelineDesc& desc) : m_Device(std::move(device))
{
    vk::PipelineCache vulkanPipelineCache;
    vk::PipelineCacheCreateInfo pipelineCacheInfo{};

    bool cacheExists = false;
    if (!desc.pipelineCachePath.empty())
    {
        cacheExists = fs::exists(desc.pipelineCachePath);
    }

    const auto& physDeviceProps = m_Device->GetVulkanPhysicalDeviceProperties();
    const auto vulkanDevice = m_Device->GetVulkanDevice();
    const auto vulkanAllocationCallbacks = m_Device->GetVulkanAllocationCallbacks();

    vk::Result vkRes;

    // Read/create pipeline cache.
    if (cacheExists)
    {
        // XXX TODO: Read cache binary file.
        // void* cacheData = nullptr;
        std::vector<u8> cacheData;

        auto cacheHeader = reinterpret_cast<vk::PipelineCacheHeaderVersionOne*>(cacheData.data());

        if ((cacheHeader->deviceID == physDeviceProps.deviceID) && (cacheHeader->vendorID == physDeviceProps.deviceID)
            && (memcmp(cacheHeader->pipelineCacheUUID, physDeviceProps.pipelineCacheUUID, VK_UUID_SIZE) == 0))
        {
            pipelineCacheInfo.setPInitialData(cacheData.data()).setInitialDataSize(cacheData.size() * sizeof(u8));
        }
        else
        {
            cacheExists = false;
        }

        vkRes = vulkanDevice.createPipelineCache(&pipelineCacheInfo, vulkanAllocationCallbacks, &vulkanPipelineCache);
        VK_CHECK(vkRes);
    }
    else
    {
        vkRes = vulkanDevice.createPipelineCache(&pipelineCacheInfo, vulkanAllocationCallbacks, &vulkanPipelineCache);
        VK_CHECK(vkRes);
    }

    // Create shader state.
    // XXX: Currently shader state is created externally. Change to creating it inside here?

    // Setup VkDescriptorSetLayouts.
    // XXX: Currently descriptor set layout is created externally. Change to creating it inside here?
    std::vector<vk::DescriptorSetLayout> vulkanDescriptorSetLayouts;
    for (const auto* layout : desc.descriptorSetLayouts)
    {
        vulkanDescriptorSetLayouts.push_back(layout->GetVulkanHandle());
    }

    // Setup push constants.
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

    // Create vulkan pipeline layout.
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
                                  .setSetLayouts(vulkanDescriptorSetLayouts)
                                  .setPushConstantRanges(pushConstantRanges);
    vkRes = vulkanDevice.createPipelineLayout(&pipelineLayoutInfo, vulkanAllocationCallbacks, &m_VulkanPipelineLayout);
    VK_CHECK(vkRes);

    if (desc.type == PipelineType::Graphics)
    {
        InitGraphicsPipeline(desc, vulkanPipelineCache);
    }
    else if (desc.type == PipelineType::Compute)
    {
        InitComputePipeline(desc, vulkanPipelineCache);
    }
    else
    {
        LOG_ERROR("Create pipeline: Unknown PipelineType enum!");
        return;
    }

    if (!m_VulkanPipeline)
    {
        return;
    }

    m_Name = desc.name;
    m_Desc = desc;
    m_Device->SetVulkanResourceName(vk::ObjectType::ePipeline, m_VulkanPipeline, desc.name);

    // Hold strong reference for referenced GPU resources?
    if (!m_Device->UseDynamicRendering())
    {
        m_ShaderState = ShaderStateRef::Create(desc.shaderState);
        for (auto descriptorLayout : desc.descriptorSetLayouts)
        {
            m_DescriptorSetLayouts.push_back(DescriptorSetLayoutRef::Create(descriptorLayout));
        }
    }

    // XXX: Save pipeline.
    if (!cacheExists && desc.pipelineCachePath.empty())
    {
        // XXX TODO: Save cache to binary file.
    }

    m_Device->GetVulkanDevice().destroyPipelineCache(vulkanPipelineCache);
}

Pipeline::~Pipeline()
{
    if (m_VulkanPipeline)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::Pipeline, m_VulkanPipeline);
    }

    if (m_VulkanPipelineLayout)
    {
        m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::PipelineLayout,
                                                             m_VulkanPipelineLayout);
    }
}

void Pipeline::InitGraphicsPipeline(const PipelineDesc& desc, vk::PipelineCache pipelineCache)
{
    auto pipelineInfo = vk::GraphicsPipelineCreateInfo().setLayout(m_VulkanPipelineLayout);

    // Shader stages.
    pipelineInfo.setStages(desc.shaderState->GetVulkanShaderStages());

    // Vertex input State.
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo();

    // Vertex attributes.
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    for (const auto& vertexAttrib : desc.vertexInput.vertexAttributes)
    {
        vertexAttributes.push_back(vk::VertexInputAttributeDescription()
                                       .setLocation(vertexAttrib.location)
                                       .setBinding(vertexAttrib.binding)
                                       .setFormat(vertexAttrib.format));
    }
    vertexInputInfo.setVertexAttributeDescriptions(vertexAttributes);

    // Vertex bindings.
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    for (const auto& vertexStream : desc.vertexInput.vertexStreams)
    {
        vertexBindings.push_back(vk::VertexInputBindingDescription()
                                     .setBinding(vertexStream.binding)
                                     .setStride(vertexStream.stride)
                                     .setInputRate((vertexStream.inputRate == VertexInputRate::PerVertex)
                                                       ? vk::VertexInputRate::eVertex
                                                       : vk::VertexInputRate::eInstance));
    }
    vertexInputInfo.setVertexBindingDescriptions(vertexBindings);

    pipelineInfo.setPVertexInputState(&vertexInputInfo);

    // Input assembly.
    auto inputAssembly
        = vk::PipelineInputAssemblyStateCreateInfo().setTopology(desc.topology).setPrimitiveRestartEnable(false);
    pipelineInfo.setPInputAssemblyState(&inputAssembly);

    // Viewport state.
    auto viewport = vk::Viewport()
                        .setX(0.0f)
                        .setY(0.0f)
                        .setWidth((float)desc.width)
                        .setHeight((float)desc.height)
                        .setMinDepth(0.0f)
                        .setMaxDepth(1.0f);
    auto scissor = vk::Rect2D().setOffset({0, 0}).setExtent({desc.width, desc.height});
    auto viewportState = vk::PipelineViewportStateCreateInfo()
                             .setViewportCount(1)
                             .setViewports(viewport)
                             .setScissorCount(1)
                             .setScissors(scissor);
    pipelineInfo.setPViewportState(&viewportState);

    // Color blend state.
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;

    if (!desc.blendState.blendStates.empty())
    {
        assert(desc.blendState.blendStates.size() == desc.renderPass->GetOutput().colorTargets.size());

        for (const auto& blendState : desc.blendState.blendStates)
        {
            auto colorBlendAttachment
                = vk::PipelineColorBlendAttachmentState()
                      // XXX: Use color write mask from blend state.
                      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                         | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                      .setBlendEnable(blendState.blendEnabled)
                      .setSrcColorBlendFactor(blendState.sourceColor)
                      .setDstColorBlendFactor(blendState.destinationColor)
                      .setColorBlendOp(blendState.colorOperation);

            if (blendState.separateBlend)
            {
                colorBlendAttachment.setSrcAlphaBlendFactor(blendState.sourceAlpha)
                    .setDstAlphaBlendFactor(blendState.destinationAlpha)
                    .setAlphaBlendOp(blendState.alphaOperation);
            }
            else
            {
                colorBlendAttachment.setSrcAlphaBlendFactor(blendState.sourceColor)
                    .setDstAlphaBlendFactor(blendState.destinationColor)
                    .setAlphaBlendOp(blendState.colorOperation);
            }

            colorBlendAttachments.push_back(colorBlendAttachment);
        }
    }
    else
    {
        for (u32 i = 0; i < desc.renderPass->GetOutput().colorTargets.size(); i++)
        {
            // Default (disabled) blend state.
            auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState().setBlendEnable(false).setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA);

            // Default (enabled) blend state.
            // auto colorBlendAttachment
            //     = vk::PipelineColorBlendAttachmentState()
            //           .setBlendEnable(true)
            //           .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            //           .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            //           .setColorBlendOp(vk::BlendOp::eAdd)
            //           .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            //           .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            //           .setAlphaBlendOp(vk::BlendOp::eAdd)
            //           .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            //                              | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

            colorBlendAttachments.push_back(colorBlendAttachment);
        }
    }

    auto colorBlendState = vk::PipelineColorBlendStateCreateInfo()
                               .setLogicOpEnable(false)
                               .setLogicOp(vk::LogicOp::eCopy)
                               .setAttachmentCount(1)
                               .setAttachments(colorBlendAttachments)
                               .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});
    pipelineInfo.setPColorBlendState(&colorBlendState);

    // Depth stencil state.
    auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo()
                                 .setDepthTestEnable(desc.depthStencilState.depthEnable)
                                 .setDepthWriteEnable(desc.depthStencilState.depthWriteEnable)
                                 .setDepthCompareOp(desc.depthStencilState.depthCompare)
                                 .setDepthBoundsTestEnable(false)
                                 .setMinDepthBounds(0.0f)
                                 .setMaxDepthBounds(0.0f);
    pipelineInfo.setPDepthStencilState(&depthStencilState);

    // Multisample state.
    auto multisampleState = vk::PipelineMultisampleStateCreateInfo()
                                .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                                .setSampleShadingEnable(false)
                                .setMinSampleShading(1.0f);
    pipelineInfo.setPMultisampleState(&multisampleState);

    // Rasterization state.
    auto rasterizationState = vk::PipelineRasterizationStateCreateInfo()
                                  .setPolygonMode(vk::PolygonMode::eFill)
                                  .setCullMode(desc.rasterizationState.cullMode)
                                  .setFrontFace(desc.rasterizationState.frontFace)
                                  .setLineWidth(1.0f)
                                  .setDepthBiasEnable(false)
                                  .setDepthClampEnable(false);
    pipelineInfo.setPRasterizationState(&rasterizationState);

    // XXX TODO: Tesssellation state.
    // auto tessellationState = vk::PipelineTessellationStateCreateInfo();
    // pipelineInfo.setPTessellationState(&tessellationState);

    // Render pass/rendering info.
    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
    if (m_Device->UseDynamicRendering())
    {
        std::vector<vk::Format> colorAttachments;
        for (const auto& colorTarget : desc.renderPass->GetOutput().colorTargets)
        {
            colorAttachments.push_back(colorTarget.format);
        }

        pipelineRenderingInfo.setViewMask(0)
            .setColorAttachmentFormats(colorAttachments)
            .setDepthAttachmentFormat(desc.renderPass->GetOutput().depthStencilFormat)
            .setStencilAttachmentFormat(vk::Format::eUndefined);

        pipelineInfo.setPNext(&pipelineRenderingInfo);
    }
    else
    {
        pipelineInfo.setRenderPass(desc.renderPass->GetVulkanHandle());
    }

    // Dynamic states.
    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    auto dynamicState = vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);
    pipelineInfo.setPDynamicState(&dynamicState);

    // Create vulkan graphics pipeline.
    auto vkRes = m_Device->GetVulkanDevice().createGraphicsPipelines(
        pipelineCache, 1, &pipelineInfo, m_Device->GetVulkanAllocationCallbacks(), &m_VulkanPipeline);
    VK_CHECK(vkRes);

    if (vkRes != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create vulkan graphics pipeline!");
        return;
    }

    m_VulkanPipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    m_RasterizationState = desc.rasterizationState;
    m_DepthStencilState = desc.depthStencilState;
    m_BlendState = desc.blendState;
    m_Type = PipelineType::Graphics;
}

void Pipeline::InitComputePipeline(const PipelineDesc& desc, vk::PipelineCache pipelineCache)
{
    assert(m_VulkanPipelineLayout);
    auto pipelineInfo = vk::ComputePipelineCreateInfo()
                            // There should only be 1 stage for a compute pipeline.
                            .setStage(desc.shaderState->GetVulkanShaderStages()[0])
                            .setLayout(m_VulkanPipelineLayout);

    auto vkRes = m_Device->GetVulkanDevice().createComputePipelines(
        pipelineCache, 1, &pipelineInfo, m_Device->GetVulkanAllocationCallbacks(), &m_VulkanPipeline);
    VK_CHECK(vkRes);
    if (vkRes != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create vulkan compute pipeline!");
        return;
    }

    m_VulkanPipelineBindPoint = vk::PipelineBindPoint::eCompute;
    m_Type = PipelineType::Compute;
}

PipelineRef VulkanDevice::CreatePipeline(const PipelineDesc& desc)
{
    PipelineRef pipelineRef = PipelineRef::Create(new Pipeline(this, desc));
    if (!pipelineRef->GetVulkanHandle())
    {
        pipelineRef = nullptr;
    }
    return pipelineRef;
}