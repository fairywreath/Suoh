#include "VulkanCommandList.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace RenderLib
{

namespace Vulkan
{

CommandList::CommandList(const VulkanContext& context, VulkanDevice* device,
                         const CommandListDesc& desc)
    : desc(desc), m_Context(context), m_pDevice(device), m_UploadManager(device)
{
    InitCommandBuffer();
}

CommandList::~CommandList()
{
    m_Context.device.destroyCommandPool(m_CommandPool);
}

CommandListHandle VulkanDevice::CreateCommandList(const CommandListDesc& desc)
{
    return CommandListHandle::Create(new CommandList(m_Context, this, desc));
}

void CommandList::Begin()
{
    m_CommandBuffer.reset();

    auto beginInfo
        = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    const auto res = m_CommandBuffer.begin(&beginInfo);
    VK_CHECK_RETURN(res);
}

void CommandList::End()
{
    // EndRenderPass();
    m_CommandBuffer.end();
}

void CommandList::CopyBuffer(Buffer* src, Buffer* dst, u32 size, u32 srcOffset, u32 dstOffset)
{
    auto copyParams
        = vk::BufferCopy().setSrcOffset(srcOffset).setDstOffset(dstOffset).setSize(size);

    m_CommandBuffer.copyBuffer(src->buffer, dst->buffer, 1, &copyParams);
}

void CommandList::CopyBufferToImage(Buffer* buffer, Image* image)
{
    assert(image->currentLayout == vk::ImageLayout::eTransferDstOptimal);

    auto imageCopy = vk::BufferImageCopy()
                         .setBufferOffset(0)
                         .setBufferRowLength(0)
                         .setBufferImageHeight(0)
                         .setImageSubresource(vk::ImageSubresourceLayers()
                                                  .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                  .setMipLevel(0)
                                                  .setBaseArrayLayer(0)
                                                  .setLayerCount(image->desc.layerCount))
                         .setImageOffset(vk::Offset3D().setX(0).setY(0).setZ(0))
                         .setImageExtent(vk::Extent3D()
                                             .setWidth(image->desc.width)
                                             .setHeight(image->desc.height)
                                             .setDepth(1));

    m_CommandBuffer.copyBufferToImage(buffer->buffer, image->image,
                                      vk::ImageLayout::eTransferDstOptimal, imageCopy);
}

void CommandList::WriteBuffer(Buffer* buffer, const void* data, u32 size, u32 dstOffset)
{
    auto chunk = m_UploadManager.GetChunk();

    memcpy(chunk->mappedMemory, data, size);
    CopyBuffer(chunk->buffer, buffer, size, 0, dstOffset);
}

void CommandList::WriteImage(Image* image, const void* data)
{
    VkDeviceSize layerSize
        = image->desc.width * image->desc.height * BytesPerTexFormat(image->desc.format);
    VkDeviceSize imageSize = layerSize * image->desc.layerCount;

    auto chunk = m_UploadManager.GetChunk();
    memcpy(chunk->mappedMemory, data, imageSize);

    TransitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal);
    CopyBufferToImage(chunk->buffer, image);
    TransitionImageLayout(image, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void CommandList::SetGraphicsState(const GraphicsState& graphicsState)
{
    const auto& renderPassDesc = graphicsState.renderPass->desc;
    const auto& framebufferDesc = graphicsState.frameBuffer->desc;

    std::vector<vk::ClearValue> clearValues;
    if (renderPassDesc.clearColor)
    {
        clearValues.push_back(
            vk::ClearValue().setColor(vk::ClearColorValue().setFloat32({0.0f, 0.0f, 0.0f, 1.0f})));
    }
    if (renderPassDesc.clearDepth)
    {
        clearValues.push_back(vk::ClearValue().setDepthStencil({1.0f, 0}));
    }

    const auto rect = vk::Rect2D().setOffset({0, 0}).setExtent({
        framebufferDesc.width,
        framebufferDesc.height,
    });

    const auto renderPassInfo = vk::RenderPassBeginInfo()
                                    .setRenderPass(graphicsState.renderPass->renderPass)
                                    .setFramebuffer(graphicsState.frameBuffer->framebuffer)
                                    .setRenderArea(rect)
                                    .setClearValues(clearValues);

    m_CommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                 graphicsState.pipeline->pipeline);
    m_CommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       graphicsState.pipeline->pipelineLayout, 0,
                                       graphicsState.descriptorSet->descriptorSet, nullptr);

    m_CurrentGraphicsState = graphicsState;
}

void CommandList::Draw(const DrawArguments& args)
{
    m_CommandBuffer.draw(args.vertexCount, args.instanceCount, args.firstVertex,
                         args.firstInstance);
}

void CommandList::DrawIndexed(const DrawArguments& args)
{
    m_CommandBuffer.drawIndexed(args.vertexCount, args.instanceCount, args.firstVertex,
                                args.vertexOffset, args.firstInstance);
}

void CommandList::DrawIndirect(u32 offset, u32 drawCount)
{
    assert(m_CurrentGraphicsState.indirectBuffer != nullptr);

    m_CommandBuffer.drawIndirect(m_CurrentGraphicsState.indirectBuffer->buffer, offset, drawCount,
                                 sizeof(vk::DrawIndirectCommand));
}

void CommandList::InitCommandBuffer()
{
    u32 queueFamily = 0;
    if (desc.queueType == QueueType::GRAPHICS)
        queueFamily = m_pDevice->GetGraphicsFamily();
    else if (desc.queueType == QueueType::COMPUTE)

        queueFamily = m_pDevice->GetComputeFamily();
    else
    {
        LOG_ERROR("InitCommandBuffer: invalid queue type!");
        return;
    }

    auto poolInfo = vk::CommandPoolCreateInfo()
                        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                        .setQueueFamilyIndex(queueFamily);

    VK_CHECK_RETURN(m_Context.device.createCommandPool(&poolInfo, nullptr, &m_CommandPool));

    auto allocInfo = vk::CommandBufferAllocateInfo()
                         .setCommandPool(m_CommandPool)
                         .setLevel(vk::CommandBufferLevel::ePrimary)
                         .setCommandBufferCount(1);

    VK_CHECK_RETURN(m_Context.device.allocateCommandBuffers(&allocInfo, &m_CommandBuffer));
}

void CommandList::BeginRenderPass(RenderPassState rpState)
{
    std::vector<vk::ClearValue> clearValues;
    if (rpState.clearColor)
        clearValues.push_back(rpState.clearColorValue);
    if (rpState.clearDepth)
        clearValues.push_back(rpState.clearDepthValue);

    const auto rect = vk::Rect2D().setOffset({0, 0}).setExtent({
        rpState.frameBuffer->desc.width,
        rpState.frameBuffer->desc.height,
    });

    const auto renderPassInfo = vk::RenderPassBeginInfo()
                                    .setRenderPass(rpState.renderPass->renderPass)
                                    .setFramebuffer(rpState.frameBuffer->framebuffer)
                                    .setRenderArea(rect)
                                    .setClearValues(clearValues);

    m_CommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void CommandList::EndRenderPass()
{
    // if (m_CurrentGraphicsState.frameBuffer)
    //{
    m_CommandBuffer.endRenderPass();
    // m_CurrentGraphicsState = {};
    //}
}

void CommandList::TransitionImageLayout(Image* image, vk::ImageLayout newLayout)
{
    if (image->currentLayout == newLayout)
        return;

    auto oldLayout = image->currentLayout;
    auto format = image->desc.format;

    auto barrier = vk::ImageMemoryBarrier()
                       .setOldLayout(oldLayout)
                       .setNewLayout(newLayout)
                       .setImage(image->image)
                       .setSubresourceRange(vk::ImageSubresourceRange()
                                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                .setBaseMipLevel(0)
                                                .setLevelCount(image->desc.mipLevels)
                                                .setBaseArrayLayer(0)
                                                .setLayerCount(image->desc.layerCount));

    if ((newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        || (format == vk::Format::eD16Unorm) || (format == vk::Format::eX8D24UnormPack32)
        || (format == vk::Format::eD32Sfloat) || (format == vk::Format::eS8Uint)
        || (format == vk::Format::eD16UnormS8Uint) || (format == vk::Format::eD24UnormS8Uint))
    {
        barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);

        if (HasStencilComponent(format))
        {
            barrier.subresourceRange.setAspectMask(barrier.subresourceRange.aspectMask
                                                   | vk::ImageAspectFlagBits::eStencil);
        }
    }
    else
    {
        barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    }

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined
        && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral)
    {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined
             && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal
             && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal
             && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined
             && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead
                                | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal
             && newLayout == vk::ImageLayout::eColorAttachmentOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    }
    else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal
             && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal
             && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eLateFragmentTests;
    }
    else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal
             && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eLateFragmentTests;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    // XXX: Wait for render pass to complete. Is this required?
    else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal
             && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits(0);
        barrier.dstAccessMask = vk::AccessFlagBits(0);

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }

    m_CommandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), {}, {},
                                    barrier);

    // XXX: Is it safe to do this without submitting the command buffer?
    image->currentLayout = newLayout;
}

} // namespace Vulkan

} // namespace RenderLib
