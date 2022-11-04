#include "SceneRenderer.h"

SceneRenderer::SceneRenderer(RenderDevice* renderDevice, Window& window)
    : m_RenderDevice(renderDevice), m_Device(renderDevice->device),
      m_FramebufferWidth(window.GetWidth()), m_FramebufferHeight(window.GetHeight())
{
}

void SceneRenderer::Init(SceneData& sceneData)
{
    m_SceneData = &sceneData;

    const auto imageCount = m_Device->GetSwapchainImageCount();

    m_UniformBuffers.resize(imageCount);
    m_Shapes.resize(imageCount);
    m_IndirectBuffers.resize(imageCount);
    m_DescriptorSets.resize(imageCount);

    const auto indirectDataSize = m_SceneData->shapes.size() * sizeof(VkDrawIndirectCommand);
    const auto shapesSize = m_SceneData->shapes.size() * sizeof(DrawData);
    constexpr auto uniformBufferSize = sizeof(m_Ubo);

    m_DescriptorLayoutDesc.imageDescriptors = {
        MakeFSImageDescriptor(sceneData.envMap.image),
        MakeFSImageDescriptor(sceneData.envMapIrradiance.image),
        MakeFSImageDescriptor(sceneData.brdfLUT.image),
    };
    m_DescriptorLayoutDesc.bufferDescriptors = {
        MakeVSFSUniformBufferDescriptor(nullptr, uniformBufferSize),
        sceneData.vertexBufferDescriptor,
        sceneData.indexBufferDescriptor,
        MakeVSStorageBufferDescriptor(nullptr, shapesSize),
        MakeVSStorageBufferDescriptor(sceneData.transformsBuffer,
                                      sceneData.transformsBuffer->desc.size),

        MakeFSStorageBufferDescriptor(sceneData.materialsBuffer,
                                      sceneData.materialsBuffer->desc.size),
    };
    m_DescriptorLayoutDesc.imageArrayDescriptors = {sceneData.materialImagesDescriptor};
    m_DescriptorLayoutDesc.poolCountMultiplier = imageCount;
    m_DescriptorLayout = m_Device->CreateDescriptorLayout(m_DescriptorLayoutDesc);

    for (unsigned int i = 0; i < imageCount; i++)
    {
        BufferDesc uboDesc;
        uboDesc.usage = BufferUsage::UNIFORM_BUFFER;
        uboDesc.size = uniformBufferSize;
        uboDesc.cpuAccess = CpuAccessMode::WRITE;
        m_UniformBuffers[i] = m_Device->CreateBuffer(uboDesc);

        BufferDesc indirectDesc;
        indirectDesc.usage = BufferUsage::INDIRECT_BUFFER;
        indirectDesc.size = indirectDataSize;
        // Mappable to CPU for now.
        indirectDesc.cpuAccess = CpuAccessMode::WRITE;
        m_IndirectBuffers[i] = m_Device->CreateBuffer(indirectDesc);

        UpdateIndirectBuffers(i);

        BufferDesc shapesDesc;
        shapesDesc.usage = BufferUsage::STORAGE_BUFFER;
        shapesDesc.size = shapesSize;
        m_Shapes[i] = m_Device->CreateBuffer(shapesDesc);
        m_RenderDevice->UploadBufferData(m_Shapes[i], m_SceneData->shapes.data(),
                                         m_SceneData->shapes.size() * sizeof(DrawData));

        // Uniform index on binding 0, shapes/drawdata on binding 3.
        // Make sure to set these before creating the set(creating will call update).
        m_DescriptorLayout->desc.bufferDescriptors[0].buffer = m_UniformBuffers[i];
        m_DescriptorLayout->desc.bufferDescriptors[3].buffer = m_Shapes[i];

        DescriptorSetDesc dsDesc;
        dsDesc.layout = m_DescriptorLayout;
        m_DescriptorSets[i] = m_Device->CreateDescriptorSet(dsDesc);
    }

    // Create depth image.
    ImageDesc depthImageDesc;
    depthImageDesc.hasDepth = true;
    depthImageDesc.hasStencil = false;
    depthImageDesc.isColorAttachment = false;
    depthImageDesc.width = m_FramebufferWidth;
    depthImageDesc.height = m_FramebufferHeight;
    depthImageDesc.format = m_Device->FindDepthFormat();
    depthImageDesc.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    depthImageDesc.tiling = vk::ImageTiling::eOptimal;

    m_DepthImage = m_Device->CreateImage(depthImageDesc);
    m_DepthImage->CreateSubresourceView();
    m_RenderDevice->TransitionImageLayout(m_DepthImage,
                                          vk::ImageLayout::eDepthStencilAttachmentOptimal);

    // Create render pass.
    RenderPassDesc rpDesc;
    rpDesc.hasDepth = true;
    rpDesc.clearColor = true;
    rpDesc.clearDepth = true;
    rpDesc.flags = RenderPassFlags::First | RenderPassFlags::Last;
    m_RenderPass = m_Device->CreateRenderPass(rpDesc);

    // Create swapchain framebuffers.
    m_SwapchainFramebuffers.resize(imageCount);
    const auto& swapchainImages = m_Device->GetSwapchainImages();
    FramebufferDesc fbDesc;
    fbDesc.renderPass = m_RenderPass;
    fbDesc.width = m_FramebufferWidth;
    fbDesc.height = m_FramebufferHeight;
    for (int i = 0; i < imageCount; i++)
    {
        fbDesc.attachments.clear();
        fbDesc.attachments.push_back(swapchainImages[i]);
        fbDesc.attachments.push_back(m_DepthImage);
        m_SwapchainFramebuffers[i] = m_Device->CreateFramebuffer(fbDesc);
    }

    // Create graphics pipeline.
    GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.descriptorSetLayout = m_DescriptorLayout->descriptorSetLayout;
    pipelineDesc.width = m_FramebufferWidth;
    pipelineDesc.height = m_FramebufferHeight;
    pipelineDesc.useDepth = true;
    pipelineDesc.useBlending = false;
    pipelineDesc.useDynamicScissorState = false;
    pipelineDesc.renderPass = m_RenderPass;

    auto vertexShader = m_Device->CreateShader({
        .fileName = "shaders/chapter07/VK01.vert",
    });
    auto fragmentShader = m_Device->CreateShader({
        .fileName = "shaders/chapter07/VK01.frag",
    });
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;

    m_GraphicsPipeline = m_Device->CreateGraphicsPipeline(pipelineDesc);
}

void SceneRenderer::RecordCommands(CommandListHandle commandList)
{
    const auto imageIndex = m_Device->GetCurrentSwapchainImageIndex();

    GraphicsState graphicsState;
    graphicsState.descriptorSet = m_DescriptorSets[imageIndex];
    graphicsState.frameBuffer = m_SwapchainFramebuffers[imageIndex];
    graphicsState.indirectBuffer = m_IndirectBuffers[imageIndex];
    graphicsState.renderPass = m_RenderPass;
    graphicsState.pipeline = m_GraphicsPipeline;

    commandList->SetGraphicsState(graphicsState);
    commandList->DrawIndirect(0, static_cast<u32>(m_SceneData->shapes.size()));
    commandList->EndRenderPass();
}

void SceneRenderer::UpdateBuffers()
{
    const auto imageIndex = m_Device->GetCurrentSwapchainImageIndex();
    const auto buffer = m_UniformBuffers[imageIndex];

    auto mappedMemory = m_Device->MapBuffer(buffer);
    memcpy(mappedMemory, &m_Ubo, sizeof(m_Ubo));
    m_Device->UnmapBuffer(buffer);
}

void SceneRenderer::UpdateIndirectBuffers(int index, bool* visibility)
{
    auto data = (vk::DrawIndirectCommand*)m_Device->MapBuffer(m_IndirectBuffers[index]);

    for (u32 i = 0; i < m_SceneData->shapes.size(); i++)
    {
        const auto j = m_SceneData->shapes[i].meshIndex;
        const auto lod = m_SceneData->shapes[i].LOD;
        data[i] = {
            .vertexCount = m_SceneData->meshData.meshes[j].GetLODIndicesCount(lod),
            .instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
            .firstVertex = 0,
            .firstInstance = i,
        };
    }

    m_Device->UnmapBuffer(m_IndirectBuffers[index]);
}
