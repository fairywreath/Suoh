#include "VulkanCommandList.h"

#include "VulkanBindings.h"
#include "VulkanDevice.h"

#include "RHIVersioning.h"

namespace SuohRHI
{

namespace Vulkan
{

// XXX: Usee command list parameters for this.
static constexpr auto C_UPLOAD_CHUNK_SIZE = 32768;
static constexpr auto C_UPLOAD_MEMORY_LIMIT = 32768 * 4;

// <= 64 KB buffers can be written using vkCmdUpdateBuffer
static constexpr auto C_COMMAND_BUFFER_WRITE_LIMIT = 65536;

VulkanCommandList::VulkanCommandList(VulkanDevice* device, const VulkanContext& context, const CommandListParameters& parameters)
    : m_pDevice(device), m_Context(context), m_CommandListParameters(parameters),
      m_UploadManager(std::make_unique<VulkanUploadManager>(device, C_UPLOAD_CHUNK_SIZE, C_UPLOAD_MEMORY_LIMIT, false)),
      m_ScratchManager(std::make_unique<VulkanUploadManager>(device, C_UPLOAD_CHUNK_SIZE, C_UPLOAD_MEMORY_LIMIT, false))
{
}

VulkanCommandList::~VulkanCommandList()
{
}

IDevice* VulkanCommandList::GetDevice()
{
    return m_pDevice;
}

Object VulkanCommandList::GetNativeObject(ObjectType type)
{
    assert(m_CurrentCommandBuffer);

    switch (type)
    {
    case ObjectTypes::Vk_CommandBuffer:
        return Object(m_CurrentCommandBuffer->commandBuffer);
    default:
        return Object(nullptr);
    }
}

void VulkanCommandList::Begin()
{
    m_CurrentCommandBuffer = m_pDevice->GetQueue(m_CommandListParameters.queueType)->GetOrCreateCommandBuffer();

    auto beginInfo = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    const auto res = m_CurrentCommandBuffer->commandBuffer.begin(&beginInfo);
    VK_CHECK_RETURN(res);

    m_CurrentCommandBuffer->referencedResources.push_back(this);

    ClearState();
}

void VulkanCommandList::End()
{
    EndRenderPass();

    // XXX: Track state?

    CommitBarriers();

    m_CurrentCommandBuffer->commandBuffer.end();

    ClearState();

    // XXX: Flush volatile buffers?
}

void VulkanCommandList::ClearState()
{
    EndRenderPass();

    m_CurrentGraphicsState = GraphicsState();

    m_CurrentPipelineLayout = vk::PipelineLayout();
    m_CurrentPipelineShaderStages = vk::ShaderStageFlagBits();
}

void VulkanCommandList::WriteBuffer(IBuffer* buffer_, const void* data, size_t dataSize, u64 destOffsetBytes)
{
    auto* buffer = checked_cast<VulkanBuffer*>(buffer_);

    assert(dataSize <= buffer->desc.byteSize);
    assert(m_CurrentCommandBuffer);

    EndRenderPass();

    m_CurrentCommandBuffer->referencedResources.push_back(buffer);

    if (dataSize <= C_COMMAND_BUFFER_WRITE_LIMIT)
    {
        if (m_EnableAutomaticBarriers)
        {

            RequireBufferState(buffer, ResourceStates::COPY_DEST);
        }
        CommitBarriers();

        i64 remaining = dataSize;
        const char* base = (const char*)data;
        while (remaining > 0)
        {
            i64 currentUpdateSize = std::min(remaining, i64(C_COMMAND_BUFFER_WRITE_LIMIT));

            // vkCmdUpdateBuffer size must be a multiple of 4.
            currentUpdateSize += currentUpdateSize % 4;
            m_CurrentCommandBuffer->commandBuffer.updateBuffer(buffer->buffer, destOffsetBytes + dataSize - remaining, currentUpdateSize,
                                                               &base[dataSize - remaining]);
            remaining -= currentUpdateSize;
        }
    }
    else
    {
        if (buffer->desc.cpuAccess != CpuAccessMode::WRITE)
        {
            VulkanBuffer* uploadBuffer;
            u64 uploadOffset;
            void* uploadMappedPtr;

            // XXX: handle versioning?
            m_UploadManager->SuballocateBuffer(dataSize, &uploadBuffer, &uploadOffset, &uploadMappedPtr,
                                               m_CurrentCommandBuffer->recordingID);
            memcpy(uploadMappedPtr, data, dataSize);
            CopyBuffer(buffer, destOffsetBytes, uploadBuffer, uploadOffset, dataSize);
        }
        else
        {
            LOG_ERROR("WriteBuffer: VulkanBuffer is mappable, writeBuffer is invalid");
        }
    }
}

void VulkanCommandList::ClearBufferUInt(IBuffer* b, u32 clearValue)
{
    auto* buffer = checked_cast<VulkanBuffer*>(b);

    assert(m_CurrentCommandBuffer);

    EndRenderPass();

    if (m_EnableAutomaticBarriers)
    {
        RequireBufferState(buffer, ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->commandBuffer.fillBuffer(buffer->buffer, 0, buffer->desc.byteSize, clearValue);
    m_CurrentCommandBuffer->referencedResources.push_back(b);
}

void VulkanCommandList::CopyBuffer(IBuffer* dest_, u64 destOffsetBytes, IBuffer* src_, u64 srcOffsetBytes, u64 dataSizeBytes)
{
    auto* dest = checked_cast<VulkanBuffer*>(dest_);
    auto* src = checked_cast<VulkanBuffer*>(src_);

    assert(destOffsetBytes + dataSizeBytes <= dest->desc.byteSize);
    assert(srcOffsetBytes + dataSizeBytes <= src->desc.byteSize);
    assert(m_CurrentCommandBuffer);

    if (dest->desc.cpuAccess != CpuAccessMode::NONE)
        m_CurrentCommandBuffer->referencedStagingBuffers.push_back(dest);
    else
        m_CurrentCommandBuffer->referencedResources.push_back(dest);

    if (src->desc.cpuAccess != CpuAccessMode::NONE)
        m_CurrentCommandBuffer->referencedStagingBuffers.push_back(src);
    else
        m_CurrentCommandBuffer->referencedResources.push_back(src);

    if (m_EnableAutomaticBarriers)
    {
        RequireBufferState(src, ResourceStates::COPY_SOURCE);
        RequireBufferState(dest, ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    auto copyRegion = vk::BufferCopy().setSize(dataSizeBytes).setSrcOffset(srcOffsetBytes).setDstOffset(destOffsetBytes);

    m_CurrentCommandBuffer->commandBuffer.copyBuffer(src->buffer, dest->buffer, {copyRegion});
}

void VulkanCommandList::SetPushConstants(const void* data, size_t byteSize)
{
    assert(m_CurrentCommandBuffer);

    m_CurrentCommandBuffer->commandBuffer.pushConstants(m_CurrentPipelineLayout, m_CurrentPipelineShaderStages, 0, u32(byteSize), data);
}

void VulkanCommandList::SetGraphicsState(const GraphicsState& state)
{
    assert(m_CurrentCommandBuffer);

    auto* fb = checked_cast<VulkanFramebuffer*>(state.framebuffer);
    auto* gp = checked_cast<VulkanGraphicsPipeline*>(state.pipeline);

    if (m_EnableAutomaticBarriers)
    {
        TrackResourcesAndBarriers(state);
    }

    bool anyBarriers = AnyBarriers();
    bool updatePipeline = false;

    if (m_CurrentGraphicsState.pipeline != state.pipeline)
    {
        m_CurrentCommandBuffer->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gp->pipeline);
        m_CurrentCommandBuffer->referencedResources.push_back(state.pipeline);
        updatePipeline = true;
    }

    if (m_CurrentGraphicsState.framebuffer != state.framebuffer || anyBarriers)
    {
        EndRenderPass();
    }

    auto desc = state.framebuffer->GetDesc();
    // XXX: Variable shading rate attachment..

    CommitBarriers();

    if (!m_CurrentGraphicsState.framebuffer)
    {
        m_CurrentCommandBuffer->commandBuffer.beginRenderPass(
            vk::RenderPassBeginInfo()
                .setRenderPass(fb->renderPass)
                .setFramebuffer(fb->framebuffer)
                .setRenderArea(vk::Rect2D()
                                   .setOffset(vk::Offset2D(0, 0))
                                   .setExtent(vk::Extent2D(fb->framebufferInfo.width, fb->framebufferInfo.height)))
                .setClearValueCount(0),
            vk::SubpassContents::eInline);

        m_CurrentCommandBuffer->referencedResources.push_back(state.framebuffer);
    }

    m_CurrentPipelineLayout = gp->pipelineLayout;
    m_CurrentPipelineShaderStages = ToVkShaderStageFlagBits(gp->shaderMask);

    // XXX: Bind extra bindpoints for compute/volatile buffers?

    // Viewports setup.
    if (!state.viewportState.viewports.empty()
        && ArraysAreDifferent(state.viewportState.viewports, m_CurrentGraphicsState.viewportState.viewports))
    {
        static_vector<vk::Viewport, MAX_VIEWPORTS> viewports;
        for (const auto& vp : state.viewportState.viewports)
        {
            viewports.push_back(ToVkViewport(vp));
        }

        m_CurrentCommandBuffer->commandBuffer.setViewport(0, u32(viewports.size()), viewports.data());
    }

    // Scissors setup.
    if (!state.viewportState.scissorRects.empty()
        && ArraysAreDifferent(state.viewportState.scissorRects, m_CurrentGraphicsState.viewportState.scissorRects))
    {
        static_vector<vk::Rect2D, MAX_VIEWPORTS> scissors;
        for (const auto& sc : state.viewportState.scissorRects)
        {
            scissors.push_back(
                vk::Rect2D(vk::Offset2D(sc.minX, sc.minY), vk::Extent2D(std::abs(sc.maxX - sc.minX), std::abs(sc.maxY - sc.minY))));
        }

        m_CurrentCommandBuffer->commandBuffer.setScissor(0, u32(scissors.size()), scissors.data());
    }

    // XXX: Blending, commandBuffer.setBlendConstants...

    // Binding index buffer.
    if (state.indexBuffer.buffer && m_CurrentGraphicsState.indexBuffer != state.indexBuffer)
    {
        m_CurrentCommandBuffer->commandBuffer.bindIndexBuffer(
            checked_cast<VulkanBuffer*>(state.indexBuffer.buffer)->buffer, state.indexBuffer.offset,
            state.indexBuffer.format == Format::R16_UINT ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

        m_CurrentCommandBuffer->referencedResources.push_back(state.indexBuffer.buffer);
    }

    // Bind vertex buffer.
    if (!state.vertexBuffers.empty() && ArraysAreDifferent(state.vertexBuffers, m_CurrentGraphicsState.vertexBuffers))
    {
        static_vector<vk::Buffer, MAX_VERTEX_ATTRIBUTES> vertexBuffers;
        static_vector<vk::DeviceSize, MAX_VERTEX_ATTRIBUTES> vertexBufferOffsets;

        for (const auto& vb : state.vertexBuffers)
        {
            vertexBuffers.push_back(checked_cast<VulkanBuffer*>(vb.buffer)->buffer);
            vertexBuffers.push_back(checked_cast<VulkanBuffer*>(vb.buffer)->buffer);
            vertexBufferOffsets.push_back(vk::DeviceSize(vb.offset));

            m_CurrentCommandBuffer->referencedResources.push_back(vb.buffer);
        }

        m_CurrentCommandBuffer->commandBuffer.bindVertexBuffers(0, u32(vertexBuffers.size()), vertexBuffers.data(),
                                                                vertexBufferOffsets.data());
    }

    // Setup indirect draw params.
    if (state.indirectParams)
    {
        m_CurrentCommandBuffer->referencedResources.push_back(state.indirectParams);
    }

    // XXX: Variable shading rate setup?

    m_CurrentGraphicsState = state;
}

void VulkanCommandList::Draw(const DrawArguments& args)
{
    assert(m_CurrentCommandBuffer);

    m_CurrentCommandBuffer->commandBuffer.draw(args.vertexCount, args.instanceCount, args.startVertexLocation, args.startInstanceLocation);
}

void VulkanCommandList::DrawIndexed(const DrawArguments& args)
{
    assert(m_CurrentCommandBuffer);

    m_CurrentCommandBuffer->commandBuffer.drawIndexed(args.vertexCount, args.instanceCount, args.startIndexLocation,
                                                      args.startVertexLocation, args.startInstanceLocation);
}

void VulkanCommandList::DrawIndirect(u32 offsetBytes)
{
    assert(m_CurrentCommandBuffer);

    // XXX: add interface to write to indirect buffer through RHI
    auto* indirectParams = checked_cast<VulkanBuffer*>(m_CurrentGraphicsState.indirectParams);

    // XXX: drawcount here is wrong, provide interface properly!
    m_CurrentCommandBuffer->commandBuffer.drawIndirect(indirectParams->buffer, offsetBytes, 1, 0);
}

void VulkanCommandList::DrawIndirectCount(u32 offsetBytes, u32 drawCount)
{
    // XXX: Implemented count indirect buffers.
    LOG_ERROR("DrawIndirectCount not implemented yet!");
    assert("DrawIndirectCount not implemented yet!");
}

void VulkanCommandList::MarkExecuted(VulkanQueue& queue, u64 submissionID)
{
    assert(m_CurrentCommandBuffer);

    m_CurrentCommandBuffer->submissionID = submissionID;

    const auto queueID = queue.GetQueueID();
    const auto recordingID = m_CurrentCommandBuffer->recordingID;

    m_CurrentCommandBuffer = nullptr;

    // XXX: Submit volatile bufers and update states?

    m_UploadManager->SubmitChunks(MakeVersion(recordingID, queueID, false), MakeVersion(submissionID, queueID, true));
    m_ScratchManager->SubmitChunks(MakeVersion(recordingID, queueID, false), MakeVersion(submissionID, queueID, true));
}

void VulkanCommandList::CopyTexture(ITexture* dest_, const TextureLayer& destLayer, ITexture* src_, const TextureLayer* srcLayer)
{
    assert(m_CurrentCommandBuffer);

    auto* dest = checked_cast<VulkanTexture*>(dest_);
    auto* src = checked_cast<VulkanTexture*>(src_);

    auto resolvedDestLayer = destLayer.Resolve(dest->desc);
    auto resolvedSrcLayer = destLayer.Resolve(src->desc);

    m_CurrentCommandBuffer->referencedResources.push_back(dest);
    m_CurrentCommandBuffer->referencedResources.push_back(src);

    auto srcSubresource = TextureSubresourceSet(resolvedSrcLayer.mipLevel, 1, resolvedSrcLayer.arrayLayer, 1);
    const auto& srcSubresourceView = src->GetSubresourceView(srcSubresource, TextureDimension::UNKNOWN);

    auto destSubresource = TextureSubresourceSet(resolvedDestLayer.mipLevel, 1, resolvedDestLayer.arrayLayer, 1);
    const auto& destSubresourceView = dest->GetSubresourceView(destSubresource, TextureDimension::UNKNOWN);

    auto imageCopy = vk::ImageCopy()
                         .setSrcSubresource(vk::ImageSubresourceLayers()
                                                .setAspectMask(srcSubresourceView.imageSubresourceRange.aspectMask)
                                                .setMipLevel(srcSubresource.baseMipLevel)
                                                .setBaseArrayLayer(srcSubresource.baseArrayLayer)
                                                .setLayerCount(srcSubresource.numArrayLayers))
                         .setSrcOffset(vk::Offset3D(resolvedSrcLayer.x, resolvedSrcLayer.y, resolvedSrcLayer.z))
                         .setDstSubresource(vk::ImageSubresourceLayers()
                                                .setAspectMask(destSubresourceView.imageSubresourceRange.aspectMask)
                                                .setMipLevel(destSubresource.baseMipLevel)
                                                .setBaseArrayLayer(destSubresource.baseArrayLayer)
                                                .setLayerCount(destSubresource.numArrayLayers))
                         .setDstOffset(vk::Offset3D(resolvedDestLayer.x, resolvedDestLayer.y, resolvedDestLayer.z))
                         .setExtent(vk::Extent3D(resolvedDestLayer.width, resolvedDestLayer.height, resolvedDestLayer.depth));

    if (m_EnableAutomaticBarriers)
    {
        RequireTextureState(src, TextureSubresourceSet(resolvedSrcLayer.mipLevel, 1, resolvedSrcLayer.arrayLayer, 1),
                            ResourceStates::COPY_SOURCE);
        RequireTextureState(dest, TextureSubresourceSet(resolvedDestLayer.mipLevel, 1, resolvedDestLayer.arrayLayer, 1),
                            ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->commandBuffer.copyImage(src->image, vk::ImageLayout::eTransferSrcOptimal, dest->image,
                                                    vk::ImageLayout::eTransferDstOptimal, {imageCopy});
}

void VulkanCommandList::CopyTexture(IStagingTexture* dest_, const TextureLayer& destLayer, ITexture* src_, const TextureLayer& srcLayer)
{
    assert(m_CurrentCommandBuffer);

    auto* src = checked_cast<VulkanTexture*>(src_);
    auto* dest = checked_cast<VulkanStagingTexture*>(dest_);

    auto resolvedDestLayer = destLayer.Resolve(dest->desc);
    auto resolvedSrcLayer = srcLayer.Resolve(src->desc);

    assert(resolvedDestLayer.depth == 1);

    vk::Extent3D srcMipSize = src->imageInfo.extent;
    srcMipSize.width = std::max(srcMipSize.width >> resolvedDestLayer.mipLevel, 1u);
    srcMipSize.height = std::max(srcMipSize.height >> resolvedDestLayer.mipLevel, 1u);

    auto destRegion = dest->GetLayerRegion(resolvedDestLayer.mipLevel, resolvedDestLayer.arrayLayer, resolvedDestLayer.z);
    assert((destRegion.offset % 0x3) == 0); // per Vulkan spec

    TextureSubresourceSet srcSubresource = TextureSubresourceSet(resolvedSrcLayer.mipLevel, 1, resolvedSrcLayer.arrayLayer, 1);

    auto imageCopy = vk::BufferImageCopy()
                         .setBufferOffset(destRegion.offset)
                         .setBufferRowLength(resolvedDestLayer.width)
                         .setBufferImageHeight(resolvedDestLayer.height)
                         .setImageSubresource(vk::ImageSubresourceLayers()
                                                  .setAspectMask(GuessImageAspectFlags(src->imageInfo.format))
                                                  .setMipLevel(resolvedSrcLayer.mipLevel)
                                                  .setBaseArrayLayer(resolvedSrcLayer.arrayLayer)
                                                  .setLayerCount(1))
                         .setImageOffset(vk::Offset3D(resolvedSrcLayer.x, resolvedSrcLayer.y, resolvedSrcLayer.z))
                         .setImageExtent(srcMipSize);

    if (m_EnableAutomaticBarriers)
    {
        RequireBufferState(dest->buffer, ResourceStates::COPY_DEST);
        RequireTextureState(src, srcSubresource, ResourceStates::COPY_SOURCE);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->commandBuffer.copyImageToBuffer(src->image, vk::ImageLayout::eTransferSrcOptimal, dest->buffer->buffer, 1,
                                                            &imageCopy);
}

void VulkanCommandList::CopyTexture(ITexture* dest_, const TextureLayer& destLayer, IStagingTexture* src_, const TextureLayer& srcLayer)
{
    assert(m_CurrentCommandBuffer);

    auto* src = checked_cast<VulkanStagingTexture*>(src_);
    auto* dest = checked_cast<VulkanTexture*>(dest_);

    auto resolvedDestLayer = destLayer.Resolve(dest->desc);
    auto resolvedSrcLayer = srcLayer.Resolve(src->desc);

    vk::Extent3D destMipSize = dest->imageInfo.extent;
    destMipSize.width = std::max(destMipSize.width >> resolvedDestLayer.mipLevel, 1u);
    destMipSize.height = std::max(destMipSize.height >> resolvedDestLayer.mipLevel, 1u);

    auto srcRegion = src->GetLayerRegion(resolvedSrcLayer.mipLevel, resolvedSrcLayer.arrayLayer, resolvedSrcLayer.z);

    assert((srcRegion.offset & 0x3) == 0); // per vulkan spec
    assert(srcRegion.size > 0);

    TextureSubresourceSet destSubresource = TextureSubresourceSet(resolvedDestLayer.mipLevel, 1, resolvedDestLayer.arrayLayer, 1);

    vk::Offset3D destOffset(resolvedDestLayer.x, resolvedDestLayer.y, resolvedDestLayer.z);

    auto imageCopy = vk::BufferImageCopy()
                         .setBufferOffset(srcRegion.offset)
                         .setBufferRowLength(resolvedSrcLayer.width)
                         .setBufferImageHeight(resolvedSrcLayer.height)
                         .setImageSubresource(vk::ImageSubresourceLayers()
                                                  .setAspectMask(GuessImageAspectFlags(dest->imageInfo.format))
                                                  .setMipLevel(resolvedDestLayer.mipLevel)
                                                  .setBaseArrayLayer(resolvedDestLayer.arrayLayer)
                                                  .setLayerCount(1))
                         .setImageOffset(destOffset)
                         .setImageExtent(destMipSize);

    if (m_EnableAutomaticBarriers)
    {
        RequireBufferState(src->buffer, ResourceStates::COPY_SOURCE);
        RequireTextureState(dest, destSubresource, ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->referencedResources.push_back(src);
    m_CurrentCommandBuffer->referencedResources.push_back(dest);

    m_CurrentCommandBuffer->commandBuffer.copyBufferToImage(src->buffer->buffer, dest->image, vk::ImageLayout::eTransferDstOptimal, 1,
                                                            &imageCopy);
}

static void ComputeMipLevelInformation(const TextureDesc& desc, u32 mipLevel, u32* widthOut, u32* heightOut, u32* depthOut)
{
    u32 width = std::max(desc.width >> mipLevel, u32(1));
    u32 height = std::max(desc.height >> mipLevel, u32(1));
    u32 depth = std::max(desc.depth >> mipLevel, u32(1));

    if (widthOut)
        *widthOut = width;
    if (heightOut)
        *heightOut = height;
    if (depthOut)
        *depthOut = depth;
}

void VulkanCommandList::WriteTexture(ITexture* dest_, u32 arrayLayer, u32 mipLevel, const void* data, size_t rowPitch, size_t depthPitch)
{
    assert(m_CurrentCommandBuffer);

    EndRenderPass();

    auto* dest = checked_cast<VulkanTexture*>(dest_);
    auto desc = dest->GetDesc();

    u32 mipWidth, mipHeight, mipDepth;
    ComputeMipLevelInformation(desc, mipLevel, &mipWidth, &mipHeight, &mipDepth);

    const FormatInfo& formatInfo = GetFormatInfo(desc.format);
    u32 deviceNumCols = (mipWidth + formatInfo.blockSize - 1) / formatInfo.blockSize;
    u32 deviceNumRows = (mipHeight + formatInfo.blockSize - 1) / formatInfo.blockSize;
    u32 deviceRowPitch = deviceNumCols * formatInfo.bytesPerBlock;
    u32 deviceMemSize = deviceRowPitch * deviceNumRows * mipDepth;

    VulkanBuffer* uploadBuffer;
    u64 uploadOffset;
    void* uploadCpuVA;
    m_UploadManager->SuballocateBuffer(deviceMemSize, &uploadBuffer, &uploadOffset, &uploadCpuVA,
                                       MakeVersion(m_CurrentCommandBuffer->recordingID, m_CommandListParameters.queueType, false));

    size_t minRowPitch = std::min(size_t(deviceRowPitch), rowPitch);
    uint8_t* mappedPtr = (uint8_t*)uploadCpuVA;
    for (u32 layer = 0; layer < mipDepth; layer++)
    {
        const uint8_t* sourcePtr = (const uint8_t*)data + depthPitch * layer;
        for (u32 row = 0; row < deviceNumRows; row++)
        {
            memcpy(mappedPtr, sourcePtr, minRowPitch);
            mappedPtr += deviceRowPitch;
            sourcePtr += rowPitch;
        }
    }

    auto imageCopy = vk::BufferImageCopy()
                         .setBufferOffset(uploadOffset)
                         .setBufferRowLength(deviceNumCols * formatInfo.blockSize)
                         .setBufferImageHeight(deviceNumRows * formatInfo.blockSize)
                         .setImageSubresource(vk::ImageSubresourceLayers()
                                                  .setAspectMask(GuessImageAspectFlags(dest->imageInfo.format))
                                                  .setMipLevel(mipLevel)
                                                  .setBaseArrayLayer(arrayLayer)
                                                  .setLayerCount(1))
                         .setImageExtent(vk::Extent3D().setWidth(mipWidth).setHeight(mipHeight).setDepth(mipDepth));

    if (m_EnableAutomaticBarriers)
    {
        RequireTextureState(dest, TextureSubresourceSet(mipLevel, 1, arrayLayer, 1), ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->referencedResources.push_back(dest);

    m_CurrentCommandBuffer->commandBuffer.copyBufferToImage(uploadBuffer->buffer, dest->image, vk::ImageLayout::eTransferDstOptimal, 1,
                                                            &imageCopy);
}

void VulkanCommandList::ResolveTexture(ITexture* dest_, const TextureSubresourceSet& destSubresources, ITexture* src_,
                                       const TextureSubresourceSet& srcSubresources)
{
    assert(m_CurrentCommandBuffer);

    EndRenderPass();

    auto* dest = checked_cast<VulkanTexture*>(dest_);
    auto* src = checked_cast<VulkanTexture*>(src_);

    TextureSubresourceSet destSR = destSubresources.Resolve(dest->desc, false);
    TextureSubresourceSet srcSR = srcSubresources.Resolve(src->desc, false);

    if (destSR.numArrayLayers != srcSR.numArrayLayers || destSR.numMipLevels != srcSR.numMipLevels)
        return;

    std::vector<vk::ImageResolve> regions;

    for (MipLevel mipLevel = 0; mipLevel < destSR.numMipLevels; mipLevel++)
    {
        vk::ImageSubresourceLayers destLayers(vk::ImageAspectFlagBits::eColor, mipLevel + destSR.baseMipLevel, destSR.baseArrayLayer,
                                              destSR.numArrayLayers);
        vk::ImageSubresourceLayers srcLayers(vk::ImageAspectFlagBits::eColor, mipLevel + srcSR.baseMipLevel, srcSR.baseArrayLayer,
                                             srcSR.numArrayLayers);

        regions.push_back(vk::ImageResolve()
                              .setSrcSubresource(srcLayers)
                              .setDstSubresource(destLayers)
                              .setExtent(vk::Extent3D(dest->desc.width >> destLayers.mipLevel, dest->desc.height >> destLayers.mipLevel,
                                                      dest->desc.depth >> destLayers.mipLevel)));
    }

    if (m_EnableAutomaticBarriers)
    {
        RequireTextureState(src, srcSR, ResourceStates::RESOLVE_SOURCE);
        RequireTextureState(dest, destSR, ResourceStates::RESOLVE_DEST);
    }
    CommitBarriers();

    m_CurrentCommandBuffer->commandBuffer.resolveImage(src->image, vk::ImageLayout::eTransferSrcOptimal, dest->image,
                                                       vk::ImageLayout::eTransferDstOptimal, regions);
}

void VulkanCommandList::BindBindingSets(const BindingSetVector& bindings, vk::PipelineLayout pipelineLayout,
                                        vk::PipelineBindPoint bindPoint)
{
    BindingVector<vk::DescriptorSet> descriptorSets;

    // XXX: Volatile uniform/constant buffers?
    static_vector<u32, C_MAX_VOLATILE_CONSTANT_BUFFERS> dynamicOffsets;

    for (const auto& binding : bindings)
    {
        const auto desc = binding->GetDesc();
        if (desc)
        {
            // Normal binding
            const auto bindingSet = checked_cast<VulkanBindingSet*>(binding);
            descriptorSets.push_back(bindingSet->descriptorSet);

            // XXX: handle volatile uniform/constant buffers?
        }
        else
        {
            // Bindless, use descriptor table.
            const auto descriptorTable = checked_cast<VulkanDescriptorTable*>(binding);
            descriptorSets.push_back(descriptorTable->descriptorSet);
        }
    }

    if (!descriptorSets.empty())
    {
        assert(m_CurrentCommandBuffer);
        m_CurrentCommandBuffer->commandBuffer.bindDescriptorSets(bindPoint, pipelineLayout, 0, u32(descriptorSets.size()),
                                                                 descriptorSets.data(), u32(dynamicOffsets.size()), dynamicOffsets.data());
    }
}

void VulkanCommandList::ClearTexture(ITexture* texture_, TextureSubresourceSet subresources, const vk::ClearColorValue& clearColorValue)
{
    assert(m_CurrentCommandBuffer);
    assert(texture_);

    EndRenderPass();

    auto* texture = checked_cast<VulkanTexture*>(texture_);

    subresources = subresources.Resolve(texture->desc, false);

    if (m_EnableAutomaticBarriers)
    {
        RequireTextureState(texture, subresources, ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                     .setBaseArrayLayer(subresources.baseArrayLayer)
                                                     .setLayerCount(subresources.numArrayLayers)
                                                     .setBaseMipLevel(subresources.baseMipLevel)
                                                     .setLevelCount(subresources.numMipLevels);

    m_CurrentCommandBuffer->commandBuffer.clearColorImage(texture->image, vk::ImageLayout::eTransferDstOptimal, &clearColorValue, 1,
                                                          &subresourceRange);
}

void VulkanCommandList::EndRenderPass()
{
    if (m_CurrentGraphicsState.framebuffer)
    {
        m_CurrentCommandBuffer->commandBuffer.endRenderPass();
        m_CurrentGraphicsState.framebuffer = nullptr;
    }
}

void VulkanCommandList::ClearTexture(ITexture* texture, TextureSubresourceSet subresources, const Color& clearColor)
{
    auto clearValue = vk::ClearColorValue().setFloat32({clearColor.r, clearColor.g, clearColor.b, clearColor.a});

    ClearTexture(texture, subresources, clearValue);
}

void VulkanCommandList::ClearTextureUint(ITexture* texture, TextureSubresourceSet subresources, u32 clearColor)
{
    auto clearColorInt = int(clearColor);

    auto clearValue = vk::ClearColorValue()
                          .setUint32({clearColor, clearColor, clearColor, clearColor})
                          .setInt32({clearColorInt, clearColorInt, clearColorInt, clearColorInt});

    ClearTexture(texture, subresources, clearValue);
}

void VulkanCommandList::ClearDepthStencilTexture(ITexture* texture_, TextureSubresourceSet subresources, bool clearDepth, float depth,
                                                 bool clearStencil, u8 stencil)
{
    assert(m_CurrentCommandBuffer);
    assert(texture_);

    EndRenderPass();

    if (!clearDepth && !clearStencil)
    {
        return;
    }

    auto* texture = checked_cast<VulkanTexture*>(texture_);

    subresources = subresources.Resolve(texture->desc, false);

    if (m_EnableAutomaticBarriers)
    {
        RequireTextureState(texture, subresources, ResourceStates::COPY_DEST);
    }
    CommitBarriers();

    vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlags();

    if (clearDepth)
        aspectFlags |= vk::ImageAspectFlagBits::eDepth;

    if (clearStencil)
        aspectFlags |= vk::ImageAspectFlagBits::eStencil;

    vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
                                                     .setAspectMask(aspectFlags)
                                                     .setBaseArrayLayer(subresources.baseArrayLayer)
                                                     .setLayerCount(subresources.numArrayLayers)
                                                     .setBaseMipLevel(subresources.baseMipLevel)
                                                     .setLevelCount(subresources.numMipLevels);

    auto clearValue = vk::ClearDepthStencilValue(depth, u32(stencil));
    m_CurrentCommandBuffer->commandBuffer.clearDepthStencilImage(texture->image, vk::ImageLayout::eTransferDstOptimal, &clearValue, 1,
                                                                 &subresourceRange);
}

void VulkanCommandList::SetResourceStatesForBindingSet(IBindingSet* bindingSet_)
{
    // Returns null if bindless.
    if (bindingSet_->GetDesc() == nullptr)
        return;

    auto* bindingSet = checked_cast<VulkanBindingSet*>(bindingSet_);

    for (auto bindingIndex : bindingSet->bindingsInTransitions)
    {
        const BindingSetItem& binding = bindingSet->desc.bindings[bindingIndex];

        switch (binding.type)
        {
            // XXX: Handle this nicely
        // case Re::Texture_SRV:
        //     RequireTextureState(checked_cast<ITexture*>(binding.resourceHandle), binding.subresources, ResourceStates::ShaderResource);
        //     break;

        // case ResourceType::Texture_UAV:
        //     RequireTextureState(checked_cast<ITexture*>(binding.resourceHandle), binding.subresources, ResourceStates::UnorderedAccess);
        //     break;

        // case ResourceType::TypedBuffer_SRV:
        // case ResourceType::StructuredBuffer_SRV:
        //     RequireBufferState(checked_cast<IBuffer*>(binding.resourceHandle), ResourceStates::ShaderResource);
        //     break;

        // case ResourceType::TypedBuffer_UAV:
        // case ResourceType::StructuredBuffer_UAV:
        //     RequireBufferState(checked_cast<IBuffer*>(binding.resourceHandle), ResourceStates::UnorderedAccess);
        //     break;
        // case BindingResourceType::CONSTANT_BUFFER:
        //    RequireBufferState(checked_cast<IBuffer*>(binding.resourceHandle), ResourceStates::CONSTANT_BUFFER);
        //    break;
        default:
            break;
        }
    }
}

void VulkanCommandList::TrackResourcesAndBarriers(const GraphicsState& state)
{
    assert(m_EnableAutomaticBarriers);

    auto* pso = checked_cast<VulkanGraphicsPipeline*>(state.pipeline);

    if (ArraysAreDifferent(state.bindings, m_CurrentGraphicsState.bindings))
    {
        for (size_t i = 0; i < state.bindings.size(); i++)
        {
            SetResourceStatesForBindingSet(state.bindings[i]);
        }
    }

    if (state.indexBuffer.buffer && state.indexBuffer.buffer != m_CurrentGraphicsState.indexBuffer.buffer)
    {
        RequireBufferState(state.indexBuffer.buffer, ResourceStates::INDEX_BUFFER);
    }

    if (ArraysAreDifferent(state.vertexBuffers, m_CurrentGraphicsState.vertexBuffers))
    {
        for (const auto& vb : state.vertexBuffers)
        {
            RequireBufferState(vb.buffer, ResourceStates::VERTEX_BUFFER);
        }
    }

    if (m_CurrentGraphicsState.framebuffer != state.framebuffer)
    {
        SetResourceStatesForFramebuffer(state.framebuffer);
    }

    if (state.indirectParams && state.indirectParams != m_CurrentGraphicsState.indirectParams)
    {
        RequireBufferState(state.indirectParams, ResourceStates::INDIRECT_ARGUMENT);
    }
}

void VulkanCommandList::RequireTextureState(ITexture* _texture, TextureSubresourceSet subresources, ResourceStates state)
{
    auto* texture = checked_cast<VulkanTexture*>(_texture);

    m_ResourceStateTracker.RequireTextureState(texture, subresources, state);
}

void VulkanCommandList::RequireBufferState(IBuffer* _buffer, ResourceStates state)
{
    auto* buffer = checked_cast<VulkanBuffer*>(_buffer);

    m_ResourceStateTracker.RequireBufferState(buffer, state);
}

bool VulkanCommandList::AnyBarriers() const
{
    return !m_ResourceStateTracker.GetBufferBarriers().empty() || !m_ResourceStateTracker.GetTextureBarriers().empty();
}

void VulkanCommandList::CommitBarriers()
{
    if (m_ResourceStateTracker.GetBufferBarriers().empty() && m_ResourceStateTracker.GetTextureBarriers().empty())
        return;

    EndRenderPass();

    std::vector<vk::ImageMemoryBarrier> imageBarriers;
    std::vector<vk::BufferMemoryBarrier> bufferBarriers;
    vk::PipelineStageFlags beforeStageFlags = vk::PipelineStageFlags(0);
    vk::PipelineStageFlags afterStageFlags = vk::PipelineStageFlags(0);

    for (const TextureBarrier& barrier : m_ResourceStateTracker.GetTextureBarriers())
    {
        VulkanResourceStateMapping before = ConvertResourceState(barrier.stateBefore);
        VulkanResourceStateMapping after = ConvertResourceState(barrier.stateAfter);

        if ((before.stageFlags != beforeStageFlags || after.stageFlags != afterStageFlags) && !imageBarriers.empty())
        {
            m_CurrentCommandBuffer->commandBuffer.pipelineBarrier(beforeStageFlags, afterStageFlags, vk::DependencyFlags(), {}, {},
                                                                  imageBarriers);

            imageBarriers.clear();
        }

        beforeStageFlags = before.stageFlags;
        afterStageFlags = after.stageFlags;

        assert(after.imageLayout != vk::ImageLayout::eUndefined);

        VulkanTexture* texture = static_cast<VulkanTexture*>(barrier.texture);

        const FormatInfo& formatInfo = GetFormatInfo(texture->desc.format);

        vk::ImageAspectFlags aspectMask = (vk::ImageAspectFlagBits)0;
        if (formatInfo.hasDepth)
            aspectMask |= vk::ImageAspectFlagBits::eDepth;
        if (formatInfo.hasStencil)
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        if (!aspectMask)
            aspectMask = vk::ImageAspectFlagBits::eColor;

        vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
                                                         .setBaseArrayLayer(barrier.entireTexture ? 0 : barrier.arrayLayer)
                                                         .setLayerCount(barrier.entireTexture ? texture->desc.layerCount : 1)
                                                         .setBaseMipLevel(barrier.entireTexture ? 0 : barrier.mipLevel)
                                                         .setLevelCount(barrier.entireTexture ? texture->desc.mipLevels : 1)
                                                         .setAspectMask(aspectMask);

        imageBarriers.push_back(vk::ImageMemoryBarrier()
                                    .setSrcAccessMask(before.accessMask)
                                    .setDstAccessMask(after.accessMask)
                                    .setOldLayout(before.imageLayout)
                                    .setNewLayout(after.imageLayout)
                                    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                    .setImage(texture->image)
                                    .setSubresourceRange(subresourceRange));
    }

    if (!imageBarriers.empty())
    {
        m_CurrentCommandBuffer->commandBuffer.pipelineBarrier(beforeStageFlags, afterStageFlags, vk::DependencyFlags(), {}, {},
                                                              imageBarriers);
    }

    beforeStageFlags = vk::PipelineStageFlags(0);
    afterStageFlags = vk::PipelineStageFlags(0);
    imageBarriers.clear();

    for (const BufferBarrier& barrier : m_ResourceStateTracker.GetBufferBarriers())
    {
        VulkanResourceStateMapping before = ConvertResourceState(barrier.stateBefore);
        VulkanResourceStateMapping after = ConvertResourceState(barrier.stateAfter);

        if ((before.stageFlags != beforeStageFlags || after.stageFlags != afterStageFlags) && !bufferBarriers.empty())
        {
            m_CurrentCommandBuffer->commandBuffer.pipelineBarrier(beforeStageFlags, afterStageFlags, vk::DependencyFlags(), {},
                                                                  bufferBarriers, {});

            bufferBarriers.clear();
        }

        beforeStageFlags = before.stageFlags;
        afterStageFlags = after.stageFlags;

        VulkanBuffer* buffer = static_cast<VulkanBuffer*>(barrier.buffer);

        bufferBarriers.push_back(vk::BufferMemoryBarrier()
                                     .setSrcAccessMask(before.accessMask)
                                     .setDstAccessMask(after.accessMask)
                                     .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                     .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                     .setBuffer(buffer->buffer)
                                     .setOffset(0)
                                     .setSize(buffer->desc.byteSize));
    }

    if (!bufferBarriers.empty())
    {
        m_CurrentCommandBuffer->commandBuffer.pipelineBarrier(beforeStageFlags, afterStageFlags, vk::DependencyFlags(), {}, bufferBarriers,
                                                              {});
    }
    bufferBarriers.clear();

    m_ResourceStateTracker.ClearBarriers();
}

void VulkanCommandList::BeginTrackingTextureState(ITexture* _texture, TextureSubresourceSet subresources, ResourceStates stateBits)
{
    VulkanTexture* texture = checked_cast<VulkanTexture*>(_texture);

    m_ResourceStateTracker.BeginTrackingTextureState(texture, subresources, stateBits);
}

void VulkanCommandList::BeginTrackingBufferState(IBuffer* _buffer, ResourceStates stateBits)
{
    VulkanBuffer* buffer = checked_cast<VulkanBuffer*>(_buffer);

    m_ResourceStateTracker.BeginTrackingBufferState(buffer, stateBits);
}

void VulkanCommandList::SetTextureState(ITexture* _texture, TextureSubresourceSet subresources, ResourceStates stateBits)
{
    VulkanTexture* texture = checked_cast<VulkanTexture*>(_texture);

    m_ResourceStateTracker.EndTrackingTextureState(texture, subresources, stateBits, false);
}

void VulkanCommandList::SetBufferState(IBuffer* _buffer, ResourceStates stateBits)
{
    VulkanBuffer* buffer = checked_cast<VulkanBuffer*>(_buffer);

    m_ResourceStateTracker.EndTrackingBufferState(buffer, stateBits, false);
}

void VulkanCommandList::SetPermanentTextureState(ITexture* _texture, ResourceStates stateBits)
{
    VulkanTexture* texture = checked_cast<VulkanTexture*>(_texture);

    m_ResourceStateTracker.EndTrackingTextureState(texture, AllSubresources, stateBits, true);
}

void VulkanCommandList::SetPermanentBufferState(IBuffer* _buffer, ResourceStates stateBits)
{
    VulkanBuffer* buffer = checked_cast<VulkanBuffer*>(_buffer);

    m_ResourceStateTracker.EndTrackingBufferState(buffer, stateBits, true);
}

ResourceStates VulkanCommandList::GetTextureSubresourceState(ITexture* _texture, ArrayLayer arrayLayer, MipLevel mipLevel)
{
    VulkanTexture* texture = checked_cast<VulkanTexture*>(_texture);

    return m_ResourceStateTracker.GetTextureSubresourceState(texture, arrayLayer, mipLevel);
}

ResourceStates VulkanCommandList::GetBufferState(IBuffer* _buffer)
{
    VulkanBuffer* buffer = checked_cast<VulkanBuffer*>(_buffer);

    return m_ResourceStateTracker.GetBufferState(buffer);
}

void VulkanCommandList::SetEnableAutomaticBarriers(bool enable)
{
    m_EnableAutomaticBarriers = enable;
}

void VulkanCommandList::SetEnableSSBOBarriersForTexture(ITexture* _texture, bool enableBarriers)
{
    VulkanTexture* texture = checked_cast<VulkanTexture*>(_texture);

    m_ResourceStateTracker.SetEnableSSBOBarriersForTexture(texture, enableBarriers);
}

void VulkanCommandList::SetEnableSSBOBarriersForBuffer(IBuffer* _buffer, bool enableBarriers)
{
    VulkanBuffer* buffer = checked_cast<VulkanBuffer*>(_buffer);

    m_ResourceStateTracker.SetEnableSSBOBarriersForBuffer(buffer, enableBarriers);
}

void VulkanCommandList::SetComputeState(const ComputeState& state)
{
}

void VulkanCommandList::Dispatch(u32 groupsX, u32 groupsY, u32 groupsZ)
{
}

void VulkanCommandList::DispatchIndirect(u32 offsetBytes)
{
}

void VulkanCommandList::BeginTimerQuery(ITimerQuery* query)
{
}

void VulkanCommandList::EndTimerQuery(ITimerQuery* query)
{
}

void VulkanCommandList::BeginMarker(const char* name)
{
}

void VulkanCommandList::EndMarker()
{
}

} // namespace Vulkan

} // namespace SuohRHI