#include "VulkanCommandBuffer.h"
#include "VulkanCommon.h"
#include "VulkanDevice.h"

#include <Core/Logger.h>

void CommandBufferManager::Init(VulkanDevice* device, u32 numThreads)
{
    m_Device = device;
    m_NumPoolsPerFrame = numThreads;

    vk::Result vkRes;

    const u32 numPools = m_NumPoolsPerFrame * GPUConstants::MaxFrames;
    m_NumUsedCommandBuffers.resize(numPools);
    m_NumUsedSecondaryCommandBuffers.resize(numPools);
    for (u32 i = 0; i < numPools; i++)
    {
        m_NumUsedCommandBuffers[i] = 0;
        m_NumUsedSecondaryCommandBuffers[i] = 0;
    }

    const u32 numCommandBuffers = numPools * m_NumCommandBuffersPerThread;
    m_CommandBuffers.resize(numCommandBuffers);

    auto vulkanDevice = m_Device->GetVulkanDevice();
    auto& threadFramePools = m_Device->GetThreadFramePools();

    LOG_INFO("Number of primary command buffers: ", numCommandBuffers);

    for (u32 i = 0; i < numCommandBuffers; i++)
    {
        const u32 frameIndex = i / (m_NumCommandBuffersPerThread * m_NumPoolsPerFrame);
        const u32 threadIndex = (i / m_NumCommandBuffersPerThread) % m_NumPoolsPerFrame;
        const u32 poolIndex = PoolFromIndices(frameIndex, threadIndex);

        auto& threadFramePool = threadFramePools[poolIndex];

        // LOG_INFO("Command buffer indices: ", frameIndex, " ", threadIndex, " ", poolIndex);

        auto commandBufferInfo = vk::CommandBufferAllocateInfo()
                                     .setCommandPool(threadFramePool.commandPool)
                                     .setLevel(vk::CommandBufferLevel::ePrimary)
                                     .setCommandBufferCount(1);

        CommandBuffer& currentCommandBuffer = m_CommandBuffers[i];

        vkRes = vulkanDevice.allocateCommandBuffers(&commandBufferInfo, &currentCommandBuffer.m_VulkanCommandBuffer);
        VK_CHECK(vkRes);

        currentCommandBuffer.m_Index = i;
        currentCommandBuffer.m_ThreadFramePools = &threadFramePool;
        currentCommandBuffer.m_IsSecondary = false;

        currentCommandBuffer.Init(m_Device);
    }

    // Create secondary command buffers.
    const u32 numSecondaryCommandBuffers = numPools * GPUConstants::SecondaryCommandBuffersCount;
    m_SecondaryCommandBuffers.reserve(numSecondaryCommandBuffers);

    LOG_INFO("Number of secondary command buffers: ", numSecondaryCommandBuffers);

    u32 cbIndex = numCommandBuffers;

    for (u32 poolIndex = 0; poolIndex < numPools; poolIndex++)
    {
        auto commandBufferInfo = vk::CommandBufferAllocateInfo()
                                     .setCommandPool(threadFramePools[poolIndex].commandPool)
                                     .setLevel(vk::CommandBufferLevel::eSecondary)
                                     .setCommandBufferCount(GPUConstants::SecondaryCommandBuffersCount);

        std::vector<vk::CommandBuffer> secondaryBuffers(GPUConstants::SecondaryCommandBuffersCount);
        vkRes = vulkanDevice.allocateCommandBuffers(&commandBufferInfo, secondaryBuffers.data());
        VK_CHECK(vkRes);

        for (u32 y = 0; y < GPUConstants::SecondaryCommandBuffersCount; y++)
        {
            CommandBuffer commandBuffer;
            commandBuffer.m_VulkanCommandBuffer = secondaryBuffers[y];
            commandBuffer.m_Index = cbIndex;
            commandBuffer.m_ThreadFramePools = &threadFramePools[poolIndex];
            commandBuffer.m_IsSecondary = true;

            commandBuffer.Init(m_Device);

            m_SecondaryCommandBuffers.push_back(commandBuffer);
        }
    }
}

void CommandBufferManager::Destroy()
{
    for (auto& commandBuffer : m_CommandBuffers)
    {
        commandBuffer.Destroy();
    }

    for (auto& commandBuffer : m_SecondaryCommandBuffers)
    {
        commandBuffer.Destroy();
    }
}

void CommandBufferManager::ResetPools(u32 frameIndex)
{
    const auto vulkanDevice = m_Device->GetVulkanDevice();
    auto& threadFramePools = m_Device->GetThreadFramePools();

    for (u32 i = 0; i < m_NumPoolsPerFrame; i++)
    {
        const u32 poolIndex = PoolFromIndices(frameIndex, i);
        vulkanDevice.resetCommandPool(threadFramePools[poolIndex].commandPool);

        m_NumUsedCommandBuffers[poolIndex] = 0;
        m_NumUsedSecondaryCommandBuffers[poolIndex] = 0;
    }
}

CommandBuffer* CommandBufferManager::GetCommandBuffer(u32 frame, u32 threadIndex, bool begin)
{
    const auto poolIndex = PoolFromIndices(frame, threadIndex);

    auto currentUsedCommandBuffer = m_NumUsedCommandBuffers[poolIndex];

    assert(currentUsedCommandBuffer <= m_NumCommandBuffersPerThread);

    if (begin)
    {
        m_NumUsedCommandBuffers[poolIndex]++;
    }

    auto commandBuffer = &m_CommandBuffers[(poolIndex * m_NumCommandBuffersPerThread) + currentUsedCommandBuffer];
    auto vulkanCommandBuffer = commandBuffer->GetVulkanCommandBuffer();

    if (begin)
    {
        // commandBuffer->Reset();
        // commandBuffer->Begin();

        // Reset timestamp queries.
        auto threadPools = commandBuffer->GetThreadFramePools();
        // threadPools->timeQueries->Reset();
        vulkanCommandBuffer.resetQueryPool(threadPools->timestampQueryPool, 0,
                                           threadPools->timeQueries->timeQueries.size());

        // Reset pipeline statistics queries.
        vulkanCommandBuffer.resetQueryPool(threadPools->pipelineStatsQuerypool, 0, GPUPipelineStatistics::Count);

        vulkanCommandBuffer.beginQuery(threadPools->pipelineStatsQuerypool, 0, vk::QueryControlFlagBits(0));
    }

    return commandBuffer;
}

CommandBuffer* CommandBufferManager::GetCommandBufferSecondary(u32 frameIndex, u32 threadIndex)
{
    const auto poolIndex = PoolFromIndices(frameIndex, threadIndex);
    const auto usedCount = m_NumUsedSecondaryCommandBuffers[poolIndex];

    assert(usedCount <= GPUConstants::SecondaryCommandBuffersCount);

    m_NumUsedSecondaryCommandBuffers[poolIndex]++;

    auto commandBuffer
        = &m_SecondaryCommandBuffers[(poolIndex * GPUConstants::SecondaryCommandBuffersCount) + usedCount];

    return commandBuffer;
}

void CommandBuffer::Init(VulkanDevice* device)
{
    m_Device = device;
    const auto vulkanDevice = m_Device->GetVulkanDevice();

    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eSampler, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eCombinedImageSampler, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eSampledImage, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBuffer, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformBuffer, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformBufferDynamic, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformTexelBuffer, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBuffer, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBufferDynamic, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageTexelBuffer, GPUConstants::CommandBufferDescriptorPoolElementSize},
        {vk::DescriptorType::eInputAttachment, GPUConstants::CommandBufferDescriptorPoolElementSize},
    };

    auto poolInfo
        = vk::DescriptorPoolCreateInfo().setMaxSets(GPUConstants::PoolSize::DescriptorSets).setPoolSizes(poolSizes);
    auto vkRes = vulkanDevice.createDescriptorPool(&poolInfo, nullptr, &m_VulkanDescriptorPool);
    VK_CHECK(vkRes);

    m_DescriptorSets.reserve(GPUConstants::MaxCommandBufferDescriptorSets);
    m_VulkanDescriptorSets.reserve(GPUConstants::MaxCommandBufferDescriptorSets);

    Reset();
}

void CommandBuffer::Destroy()
{
    m_IsRecording = false;
    // Reset();
    m_Device->GetVulkanDevice().destroyDescriptorPool(m_VulkanDescriptorPool);
}

void CommandBuffer::Reset()
{
    m_IsRecording = false;

    m_CurrentRenderPass = nullptr;
    m_CurrentFramebuffer = nullptr;
    m_CurrentPipeline = nullptr;
    m_CurrentCommand = 0;

    const auto vulkanDevice = m_Device->GetVulkanDevice();
    vulkanDevice.resetDescriptorPool(m_VulkanDescriptorPool);

    m_VulkanDescriptorSets.clear();
    m_DescriptorSets.clear();
}

void CommandBuffer::Begin()
{
    if (!m_IsRecording)
    {
        auto beginInfo = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        m_VulkanCommandBuffer.begin(beginInfo);

        m_IsRecording = true;
    }
    else
    {
        LOG_WARN("Begin call to command buffer that is already recording!");
    }
}

void CommandBuffer::BeginSecondary(RenderPass* currentRenderPass, Framebuffer* currentFramebuffer)
{
    if (!m_IsRecording)
    {
        assert(currentRenderPass);
        assert(currentFramebuffer);

        auto inheritanceInfo = vk::CommandBufferInheritanceInfo()
                                   .setRenderPass(currentRenderPass->GetVulkanHandle())
                                   .setSubpass(0)
                                   .setFramebuffer(currentFramebuffer->GetVulkanHandle());

        auto beginInfo = vk::CommandBufferBeginInfo()
                             .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit
                                       | vk::CommandBufferUsageFlagBits::eRenderPassContinue)
                             .setPInheritanceInfo(&inheritanceInfo);

        m_VulkanCommandBuffer.begin(beginInfo);

        m_IsRecording = true;

        m_CurrentRenderPass = currentRenderPass;
        m_CurrentFramebuffer = currentFramebuffer;
    }
    else
    {
        LOG_WARN("Begin call to secondary command buffer that is already recording");
    }
}

void CommandBuffer::End()
{
    if (m_IsRecording)
    {
        m_VulkanCommandBuffer.end();

        m_IsRecording = false;
    }
    else
    {
        LOG_WARN("End call to command buffer that is not recording!");
    }
}

void CommandBuffer::EndCurrentRenderPass()
{
    assert(m_IsRecording);

    if (m_IsRecording && (m_CurrentRenderPass != nullptr))
    {
        if (m_Device->UseDynamicRendering())
        {
            m_VulkanCommandBuffer.endRendering();
        }
        else
        {
            m_VulkanCommandBuffer.endRenderPass();
        }

        m_CurrentRenderPass = nullptr;
    }
}

void CommandBuffer::BindRenderPass(RenderPass* renderPass, Framebuffer* framebuffer, bool useSecondary)
{
    assert(m_IsRecording);

    assert(renderPass);
    assert(framebuffer);

    if ((m_CurrentRenderPass != nullptr) && (m_CurrentRenderPass != renderPass))
    {
        // EndCurrentRenderPass();
        return;
    }

    const auto& renderPassOutput = renderPass->GetOutput();
    const auto containsDepthStencil = framebuffer->ContainsDepthStencilAttachment();

    if (renderPass != m_CurrentRenderPass)
    {
        if (m_Device->UseDynamicRendering())
        {

            std::vector<vk::RenderingAttachmentInfo> colorAttachmentInfos(framebuffer->NumColorAttachments());

            for (u32 i = 0; i < framebuffer->NumColorAttachments(); i++)
            {
                auto texture = framebuffer->GetColorAttachment(i);
                texture->SetResourceState(ResourceState::RenderTarget);

                vk::AttachmentLoadOp colorOp;
                switch (renderPassOutput.colorTargets[i].operation)
                {
                case RenderPassOperation::Load:
                    colorOp = vk::AttachmentLoadOp::eLoad;
                    break;
                case RenderPassOperation::Clear:
                    colorOp = vk::AttachmentLoadOp::eClear;
                    break;
                default:
                    colorOp = vk::AttachmentLoadOp::eDontCare;
                    break;
                }

                auto& colorAttachmentInfo = colorAttachmentInfos[i];

                colorAttachmentInfo.setImageView(texture->GetVulkanView())
                    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
                    // .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                    .setResolveMode(vk::ResolveModeFlagBits::eNone)
                    .setLoadOp(colorOp)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setClearValue((renderPassOutput.colorTargets[i].operation == RenderPassOperation::Clear)
                                       ? m_ClearValues[i]
                                       : vk::ClearValue());
            }

            auto depthAttachmentInfo = vk::RenderingAttachmentInfo();

            if (containsDepthStencil)
            {
                auto texture = framebuffer->GetDepthStenciAttachment();
                texture->SetResourceState(ResourceState::DepthWrite);

                vk::AttachmentLoadOp depthOp;
                switch (renderPassOutput.depthOperation)
                {
                case RenderPassOperation::Load:
                    depthOp = vk::AttachmentLoadOp::eLoad;
                    break;
                case RenderPassOperation::Clear:
                    depthOp = vk::AttachmentLoadOp::eClear;
                    break;
                default:
                    depthOp = vk::AttachmentLoadOp::eDontCare;
                    break;
                }

                depthAttachmentInfo.setImageView(texture->GetVulkanView())
                    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
                    .setResolveMode(vk::ResolveModeFlagBits::eNone)
                    .setLoadOp(depthOp)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setClearValue((renderPassOutput.depthOperation == RenderPassOperation::Clear)
                                       ? m_ClearValues[GPUConstants::DepthStencilClearIndex]
                                       : vk::ClearValue());
            }

            auto renderingInfo = vk::RenderingInfo()
                                     .setFlags(useSecondary ? vk::RenderingFlagBits::eContentsSecondaryCommandBuffers
                                                            : vk::RenderingFlagBits(0))
                                     .setRenderArea(vk::Rect2D().setOffset({0, 0}).setExtent(
                                         {framebuffer->GetWidth(), framebuffer->GetHeight()}))
                                     //   .setLayerCount(1)
                                     //   .setViewMask(renderPass->multiViewMask)
                                     .setColorAttachments(colorAttachmentInfos)
                                     .setPDepthAttachment(containsDepthStencil ? &depthAttachmentInfo : nullptr)
                                     .setPStencilAttachment(nullptr);

            // XXX: Variable shading rate.
            // auto shadingRateInfo = vk::RenderingFragmentShadingRateAttachmentInfoKHR();

            m_VulkanCommandBuffer.beginRendering(renderingInfo);
        }
        else
        {
            u32 clearValuesCount = renderPassOutput.colorTargets.size();
            if ((renderPassOutput.depthStencilFormat != vk::Format::eUndefined)
                && (renderPassOutput.depthOperation == RenderPassOperation::Clear))
            {
                m_ClearValues[clearValuesCount++] = m_ClearValues[GPUConstants::DepthStencilClearIndex];
            }

            auto renderPassBeginInfo = vk::RenderPassBeginInfo()
                                           .setFramebuffer(framebuffer->GetVulkanHandle())
                                           .setRenderPass(renderPass->GetVulkanHandle())
                                           .setRenderArea(vk::Rect2D().setOffset({0, 0}).setExtent(
                                               {framebuffer->GetWidth(), framebuffer->GetHeight()}))
                                           .setClearValueCount(clearValuesCount)
                                           .setPClearValues(m_ClearValues.data());

            m_VulkanCommandBuffer.beginRenderPass(renderPassBeginInfo,
                                                  useSecondary ? vk::SubpassContents::eSecondaryCommandBuffers
                                                               : vk::SubpassContents::eInline);
        }
    }

    m_CurrentRenderPass = renderPass;
    m_CurrentFramebuffer = framebuffer;
}

void CommandBuffer::BindPipeline(Pipeline* pipeline)
{
    assert(pipeline);
    m_VulkanCommandBuffer.bindPipeline(pipeline->GetVulkanBindPoint(), pipeline->GetVulkanHandle());
    m_CurrentPipeline = pipeline;
}

void CommandBuffer::BindDescriptorSets(const std::vector<DescriptorSet*>& descriptorSets)
{
    std::vector<u32> offsets;

    // Search for dynamic uniform buffers.
    for (auto set : descriptorSets)
    {
        m_VulkanDescriptorSets.push_back(set->GetVulkanHandle());

        const auto setLayout = set->GetLayout();
        const auto& bindings = setLayout->GetBindings();
        const auto& setResources = set->GetResources();

        for (u32 i = 0; i < setLayout->NumBindings(); i++)
        {
            auto& binding = bindings[i];

            auto resourceIndex = i;
            // auto resourceIndex = setShaderBindings[i];

            if (binding.type == vk::DescriptorType::eUniformBuffer)
            {
                auto data = setResources[resourceIndex].data;
                assert(std::holds_alternative<GPUResource*>(data));

                auto resourcePtr = get<GPUResource*>(data);

                auto buffer = static_cast<Buffer*>(resourcePtr);

                offsets.push_back(buffer->GetGlobalOffset());
            }
        }
    }

    const u32 firstSet = 0;

    m_VulkanCommandBuffer.bindDescriptorSets(m_CurrentPipeline->GetVulkanBindPoint(),
                                             m_CurrentPipeline->GetVulkanLayout(), firstSet, descriptorSets.size(),
                                             m_VulkanDescriptorSets.data(), offsets.size(), offsets.data());

    if (m_Device->UseBindless())
    {
        auto bindlessDescriptorSet = m_Device->GetVulkanBindlessDescriptorSet();
        m_VulkanCommandBuffer.bindDescriptorSets(m_CurrentPipeline->GetVulkanBindPoint(),
                                                 m_CurrentPipeline->GetVulkanLayout(), 0, 1, &bindlessDescriptorSet, 0,
                                                 nullptr);
    }
}

// void CommandBuffer::BindLocalDescriptorSets(const std::vector<DescriptorSet*>& descriptorSets)
// {
// }

void CommandBuffer::SetViewport(const Viewport& viewport)
{
    auto vulkanViewport = vk::Viewport()
                              .setX(viewport.rect.x * 1.0f)
                              .setY(viewport.rect.y * 1.0f)
                              .setWidth(viewport.rect.width * 1.0f)
                              .setHeight(viewport.rect.height * 1.0f)
                              .setMinDepth(viewport.minDepth)
                              .setMaxDepth(viewport.maxDepth);
    m_VulkanCommandBuffer.setViewport(0, vulkanViewport);
}

void CommandBuffer::SetScissor(const Rect2DInt& rect)
{
    auto scissor = vk::Rect2D().setOffset({rect.x, rect.y}).setExtent({rect.width, rect.height});
    m_VulkanCommandBuffer.setScissor(0, scissor);
}

void CommandBuffer::Clear(f32 red, f32 green, f32 blue, f32 alpha, u32 attachmentIndex)
{
    assert(attachmentIndex < m_ClearValues.size());
    m_ClearValues[attachmentIndex].setColor({red, green, blue, alpha});
}

void CommandBuffer::ClearDepthStencil(f32 depth, u8 stencil)
{
    m_ClearValues[GPUConstants::DepthStencilClearIndex].setDepthStencil({depth, stencil});
}

void CommandBuffer::PushConstants(Pipeline* pipeline, u32 offset, u32 size, void* data)
{
    m_VulkanCommandBuffer.pushConstants(pipeline->GetVulkanLayout(), vk::ShaderStageFlagBits::eAll, offset, size, data);
}

void CommandBuffer::Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance)
{
    m_VulkanCommandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance)
{
    m_VulkanCommandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::DrawIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride)
{
    m_VulkanCommandBuffer.drawIndexedIndirect(buffer->GetVulkanHandle(), offset, drawCount, stride);
}

void CommandBuffer::DrawIndirectCount(Buffer* argumentBuffer, u32 argumentOffset, Buffer* countBuffer, u32 countOffset,
                                      u32 maxDraws, u32 stride)
{
    m_VulkanCommandBuffer.drawIndirectCount(argumentBuffer->GetVulkanHandle(), argumentOffset,
                                            countBuffer->GetVulkanHandle(), countOffset, maxDraws, stride);
}

void CommandBuffer::DrawIndexedIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride)
{
    m_VulkanCommandBuffer.drawIndexedIndirect(buffer->GetVulkanHandle(), offset, drawCount, stride);
}

void CommandBuffer::DrawMeshTask(u32 taskCount, u32 firstTask)
{
    m_VulkanCommandBuffer.drawMeshTasksNV(taskCount, firstTask);
}

void CommandBuffer::DrawMeshTaskIndirectCount(Buffer* argumentBuffer, u32 argumentOffset, Buffer* countBuffer,
                                              u32 countOffset, u32 maxDraws, u32 stride)
{
    m_VulkanCommandBuffer.drawMeshTasksIndirectCountNV(argumentBuffer->GetVulkanHandle(), argumentOffset,
                                                       countBuffer->GetVulkanHandle(), countOffset, maxDraws, stride);
}

void CommandBuffer::DrawMeshTaskIndirect(Buffer* buffer, u32 offset, u32 drawCount, u32 stride)
{
    m_VulkanCommandBuffer.drawMeshTasksIndirectNV(buffer->GetVulkanHandle(), offset, drawCount, stride);
}

void CommandBuffer::Dispatch(u32 groupX, u32 groupY, u32 groupZ)
{
    m_VulkanCommandBuffer.dispatch(groupX, groupY, groupZ);
}

void CommandBuffer::DispatchIndirect(Buffer* buffer, u32 offset)
{
    m_VulkanCommandBuffer.dispatchIndirect(buffer->GetVulkanHandle(), offset);
}

void CommandBuffer::CopyBuffer(Buffer* src, Buffer* dst, u32 size, u32 srcOffset, u32 dstOffset)
{
    auto copyParams = vk::BufferCopy().setSrcOffset(srcOffset).setDstOffset(dstOffset).setSize(size);

    m_VulkanCommandBuffer.copyBuffer(src->GetVulkanHandle(), dst->GetVulkanHandle(), 1, &copyParams);
}

void CommandBuffer::CopyBufferToTexture(Buffer* buffer, Texture* texture)
{
    // assert(image->currentLayout == vk::ImageLayout::eTransferDstOptimal);

    auto imageCopy = vk::BufferImageCopy()
                         .setBufferOffset(0)
                         .setBufferRowLength(0)
                         .setBufferImageHeight(0)
                         .setImageSubresource(vk::ImageSubresourceLayers()
                                                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                  .setMipLevel(0)
                                                  .setBaseArrayLayer(0)
                                                  .setLayerCount(texture->GetArrayLayerCount()))
                         .setImageOffset(vk::Offset3D().setX(0).setY(0).setZ(0))
                         .setImageExtent(vk::Extent3D()
                                             .setWidth(texture->GetWidth())
                                             .setHeight(texture->GetHeight())
                                             .setDepth(texture->GetDepth()));

    m_VulkanCommandBuffer.copyBufferToImage(buffer->GetVulkanHandle(), texture->GetVulkanHandle(),
                                            vk::ImageLayout::eTransferDstOptimal, imageCopy);
}

// void CommandBuffer::CopyTexture(Texture* src, Texture* dst, ResourceState dstState)
// {
// }

void CommandBuffer::UploadBufferData(Buffer* buffer, const void* bufferData, u32 size, Buffer* stagingBuffer,
                                     size_t stagingBufferOffset)
{

    auto mappedData = static_cast<u8*>(stagingBuffer->GetMappedData());

    memcpy(mappedData + stagingBufferOffset, bufferData, size);

    CopyBuffer(buffer, stagingBuffer, size, stagingBufferOffset, 0);

    // XXX: Buffer barrier here.
}