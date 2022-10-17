#include "VulkanDevice.h"

#include "VulkanUtils.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanDevice::VulkanDevice(const DeviceDesc& desc) : m_DeviceManager(m_Instance)
{
    /*
     * XXX: Ideally device manager should create Device(this class) types to handle
     * multi GPU/devices rendering.
     * But current RHI does not have an "Instance/DeviceManager" resource type, hence it can only use
     * 1 physical device. DeviceManager is implemented internally in the RHI however, and wrap it inside
     * this class as well.bin
     */
    auto deviceContext = m_DeviceManager.CreateDevice(desc);

    m_GraphicsQueue = deviceContext.graphicsQueue;
    m_GraphicsFamily = deviceContext.graphicsFamily;
    m_PresentQueue = deviceContext.presentQueue;
    m_PresentFamily = deviceContext.presentFamily;
    m_ComputeQueue = deviceContext.computeQueue;
    m_ComputeFamily = deviceContext.computeFamily;

    m_Context.device = deviceContext.device;
    m_Context.physicalDevice = deviceContext.physicalDevice;
    m_Context.instance = m_Instance.GetVkInstance();
    m_Context.surface = m_DeviceManager.GetVkSurfaceKHR();

    m_Swapchain.InitSwapchain(this, m_Context, desc.framebufferWidth, desc.framebufferHeight);

    // Create queues. Currently this is defaulted to always create a graphics queue and a compute queue only.
    m_Queues[u32(CommandQueue::GRAPHICS)]
        = std::make_unique<VulkanQueue>(m_Context, m_GraphicsQueue, CommandQueue::GRAPHICS, m_GraphicsFamily);
    m_Queues[u32(CommandQueue::PRESENT)] = std::make_unique<VulkanQueue>(m_Context, m_PresentQueue, CommandQueue::PRESENT, m_PresentFamily);

    // Create pipeline cache.
    auto pipelineCacheInfo = vk::PipelineCacheCreateInfo();
    VK_CHECK_RETURN(m_Context.device.createPipelineCache(&pipelineCacheInfo, nullptr, &m_Context.pipelineCache));

    InitAllocator();
}

VulkanDevice::~VulkanDevice()
{
    vmaDestroyAllocator(m_Context.allocator);

    if (m_Context.pipelineCache)
    {
        m_Context.device.destroyPipelineCache(m_Context.pipelineCache);
    }

    for (auto& q : m_Queues)
    {
        q.reset();
    }

    m_Swapchain.Destroy();
    m_Context.device.destroy();
}

GraphicsAPI VulkanDevice::GetGraphicsAPI() const
{
    return GraphicsAPI::VULKAN;
}

Object VulkanDevice::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_Instance:
        return Object(m_Instance.GetVkInstance());
    case ObjectTypes::Vk_PhysicalDevice:
        return Object(m_Context.physicalDevice);
    case ObjectTypes::Vk_Device:
        return Object(m_Context.device);
    default:
        return Object(nullptr);
    }
}

BufferHandle VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    auto* buffer = new VulkanBuffer(m_Context);
    buffer->desc = desc;

    vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;

    // XXX: Handle buffer enum flags more nicely. Use std::bitsets?
    if (u32(desc.usage & BufferUsage::VERTEX_BUFFER))
        usageFlags |= vk::BufferUsageFlagBits::eVertexBuffer;
    if (u32(desc.usage & BufferUsage::INDEX_BUFFER))
        usageFlags |= vk::BufferUsageFlagBits::eIndexBuffer;
    if (u32(desc.usage & BufferUsage::INDIRECT_BUFFER))
        usageFlags |= vk::BufferUsageFlagBits::eIndirectBuffer;
    if (u32(desc.usage & BufferUsage::UNIFORM_BUFFER))
        usageFlags |= vk::BufferUsageFlagBits::eUniformBuffer;
    if (u32(desc.usage & BufferUsage::STORAGE_BUFFER))
        usageFlags |= vk::BufferUsageFlagBits::eStorageBuffer;

    u64 size = desc.byteSize;

    if (size < 65536)
    {
        // <= 64kb buffers can be done inline using vkCmdUpdateBuffer. Size must be multiple of 4 however.
        size += size % 4;
    }

    auto bufferInfo = vk::BufferCreateInfo().setSize(size).setUsage(usageFlags).setSharingMode(vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    // Set memory property flags based on cpuAccess.
    // allocInfo.requiredFlags....

    // XXX: Casting from vk:: to Vk pointer for vma calls, alternatives to reinterpret_cast>
    auto res = vmaCreateBuffer(m_Context.allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocInfo,
                               reinterpret_cast<VkBuffer*>(&buffer->buffer), &buffer->allocation, nullptr);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create buffer with error code ", res);
        return nullptr;
    }

    return BufferHandle::Create(buffer);
}

void* VulkanDevice::MapBuffer(IBuffer* b, CpuAccessMode accessFlags)
{
    auto* buffer = checked_cast<VulkanBuffer*>(b);
    return MapBuffer(buffer, accessFlags, 0, buffer->desc.byteSize);
}

void VulkanDevice::UnmapBuffer(IBuffer* b)
{
    auto* buffer = checked_cast<VulkanBuffer*>(b);
    vmaUnmapMemory(m_Context.allocator, buffer->allocation);

    buffer->mappedMemory = nullptr;

    // XXX: Barrier?
}

BufferHandle VulkanDevice::CreateHandleForNativeBuffer(ObjectType objectType, Object buffer, const BufferDesc& desc)
{
    // if (!buffer.handle)
    // return nullptr;

    if (objectType != ObjectTypes::Vk_Buffer)
        return nullptr;

    auto* bufferPtr = new VulkanBuffer(m_Context);

    // XXX: Check nicely, this might throw.
    bufferPtr->buffer = VkBuffer(std::get<void*>(buffer.handle));
    bufferPtr->desc = desc;
    bufferPtr->managed = false;

    return BufferHandle();
}

TextureHandle VulkanDevice::CreateTexture(const TextureDesc& desc)
{
    auto* texture = new VulkanTexture(m_Context);
    texture->managed = true;

    FillTextureInfo(texture, desc);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    // XXX: Set to peoprty bits device only?.

    auto res = vmaCreateImage(m_Context.allocator, reinterpret_cast<VkImageCreateInfo*>(&texture->imageInfo), &allocInfo,
                              reinterpret_cast<VkImage*>(&texture->image), &texture->allocation, nullptr);

    return TextureHandle::Create(texture);
}

TextureHandle VulkanDevice::CreateHandleForNativeTexture(ObjectType objectType, Object tex, const TextureDesc& desc)
{
    if (std::get<void*>(tex.handle) == nullptr)
        return nullptr;

    if (objectType != ObjectTypes::Vk_Image)
        return nullptr;

    vk::Image image(VkImage(std::get<void*>(tex.handle)));

    auto* texture = new VulkanTexture(m_Context);
    FillTextureInfo(texture, desc);

    texture->image = image;
    texture->managed = false;

    return TextureHandle::Create(texture);
}

StagingTextureHandle VulkanDevice::CreateStagingTexture(const TextureDesc& desc, CpuAccessMode cpuAccess)
{
    assert(cpuAccess != CpuAccessMode::NONE);

    auto* texture = new VulkanStagingTexture();
    texture->desc = desc;
    texture->PopulateLayerRegions();

    BufferDesc bufDesc;
    bufDesc.byteSize = u32(texture->GetBufferSize());
    assert(bufDesc.byteSize > 0);
    bufDesc.cpuAccess = cpuAccess;

    auto bufferHandle = CreateBuffer(bufDesc);
    texture->buffer = checked_cast<VulkanBuffer*>(bufferHandle.Get());

    if (!texture->buffer)
    {
        delete texture;
        return nullptr;
    }

    return StagingTextureHandle::Create(texture);
}

void* VulkanDevice::MapStagingTexture(IStagingTexture* texture_, const TextureLayer& layer, CpuAccessMode access, size_t* outRowPitch)
{
    assert(layer.x == 0);
    assert(layer.y == 0);
    assert(access != CpuAccessMode::NONE);

    auto* texture = checked_cast<VulkanStagingTexture*>(texture_);
    auto resolvedLayer = layer.Resolve(texture->desc);
    auto region = texture->GetLayerRegion(resolvedLayer.mipLevel, resolvedLayer.arrayLayer, resolvedLayer.z);

    // Per Vulkan spec.
    assert((region.offset & 0x3) == 0);
    assert(region.size > 0);

    const auto& formatInfo = GetFormatInfo(texture->desc.format);

    // assert(outRowPitch);
    // auto wInBlocks = resolvedLayer.width / formatInfo.blockSize;
    //*outRowPitch = wInBlocks * formatInfo.bytesPerBlock;

    return MapBuffer(texture->buffer, access, region.offset, region.size);
}

void VulkanDevice::UnmapStagingTexture(IStagingTexture* texture_)
{
    auto* texture = checked_cast<VulkanStagingTexture*>(texture_);

    UnmapBuffer(texture->buffer);
}

ShaderHandle VulkanDevice::CreateShader(const ShaderDesc& desc, const void* binary, size_t binarySize)
{
    auto* shader = new VulkanShader(m_Context);

    shader->desc = desc;
    shader->stageFlagBits = ToVkShaderStageFlagBits(desc.shaderType);

    auto shaderInfo = vk::ShaderModuleCreateInfo().setCodeSize(binarySize).setPCode((const u32*)binary);

    const auto res = m_Context.device.createShaderModule(&shaderInfo, nullptr, &shader->shaderModule);
    VK_CHECK_RETURN_NULL(res);

    // XXX: Set debug name?

    return ShaderHandle::Create(shader);
}

ShaderLibraryHandle VulkanDevice::CreateShaderLibrary(const void* binary, size_t binarySize)
{
    auto* library = new VulkanShaderLibrary(m_Context);

    auto shaderInfo = vk::ShaderModuleCreateInfo().setCodeSize(binarySize).setPCode((const u32*)binary);

    const auto res = m_Context.device.createShaderModule(&shaderInfo, nullptr, &library->shaderModule);
    VK_CHECK_RETURN_NULL(res);

    return ShaderLibraryHandle::Create(library);
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    auto* sampler = new VulkanSampler(m_Context);

    const bool anisotropyEnable = desc.maxAnisotropy > 1.0f;

    sampler->desc = desc;
    sampler->samplerInfo = vk::SamplerCreateInfo()
                               .setMagFilter(ToMagFilter(desc.filter))
                               .setMinFilter(ToMinFilter(desc.filter))
                               .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                               .setAddressModeU(ToSamplerAddressMode(desc.addressU))
                               .setAddressModeV(ToSamplerAddressMode(desc.addressV))
                               .setAddressModeW(ToSamplerAddressMode(desc.addressW))
                               .setMipLodBias(desc.mipLODBias)
                               .setAnisotropyEnable(anisotropyEnable)
                               .setMaxAnisotropy(anisotropyEnable ? desc.maxAnisotropy : 1.f)
                               .setCompareEnable(desc.reductionMode == SamplerReductionMode::COMPARISON)
                               .setCompareOp(vk::CompareOp::eLess)
                               .setMinLod(0.f)
                               .setMaxLod(std::numeric_limits<float>::max())
                               .setBorderColor(ToSamplerBorderColor(desc.borderColor));

    vk::SamplerReductionModeCreateInfoEXT samplerReductionCreateInfo;
    if (desc.reductionMode == SamplerReductionMode::MIN || desc.reductionMode == SamplerReductionMode::MAX)
    {
        vk::SamplerReductionModeEXT reductionMode
            = desc.reductionMode == SamplerReductionMode::MAX ? vk::SamplerReductionModeEXT::eMax : vk::SamplerReductionModeEXT::eMin;
        samplerReductionCreateInfo.setReductionMode(reductionMode);
        sampler->samplerInfo.setPNext(&samplerReductionCreateInfo);
    }

    const auto res = m_Context.device.createSampler(&sampler->samplerInfo, nullptr, &sampler->sampler);
    VK_CHECK_RETURN_NULL(res);

    return SamplerHandle::Create(sampler);
}

InputLayoutHandle VulkanDevice::CreateInputLayout(const VertexAttributeDesc* attributeDesc, u32 attributeCount, IShader* vertexShader)
{
    auto* layout = new VulkanInputLayout();

    int totalAttributeArraySize = 0;
    std::unordered_map<u32, vk::VertexInputBindingDescription> bindingsMap;

    for (u32 i = 0; i < attributeCount; i++)
    {
        const auto& desc = attributeDesc[i];

        totalAttributeArraySize += desc.arraySize;

        if (!bindingsMap.contains(desc.bufferIndex))
        {
            bindingsMap[desc.bufferIndex]
                = vk::VertexInputBindingDescription()
                      .setBinding(desc.bufferIndex)
                      .setStride(desc.elementStride)
                      .setInputRate(desc.isInstanced ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex);
        }
        else
        {
            assert(bindingsMap[desc.bufferIndex].stride == desc.elementStride);
            assert(bindingsMap[desc.bufferIndex].inputRate
                   == (desc.isInstanced ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex));
        }
    }

    for (const auto& b : bindingsMap)
    {
        layout->bindingDesc.push_back(b.second);
    }

    layout->inputDesc.resize(attributeCount);
    layout->attributeDesc.resize(totalAttributeArraySize);

    u32 attributeLocation = 0;
    for (u32 i = 0; i < attributeCount; i++)
    {
        const auto& in = attributeDesc[i];
        layout->inputDesc[i] = in;

        u32 elementSizeBytes = GetFormatInfo(in.format).bytesPerBlock;

        u32 bufferOffset = 0;
        for (u32 slot = 0; slot < in.arraySize; slot++)
        {
            auto& outAttrib = layout->attributeDesc[attributeLocation];

            outAttrib.location = attributeLocation;
            outAttrib.binding = in.bufferIndex;
            outAttrib.format = ToVkFormat(in.format);
            outAttrib.offset = bufferOffset + in.offSet;
            bufferOffset += elementSizeBytes;

            attributeLocation++;
        }
    }

    return InputLayoutHandle::Create(layout);
}

EventQueryHandle VulkanDevice::CreateEventQuery()
{
    return EventQueryHandle();
}

void VulkanDevice::SetEventQuery(IEventQuery* query, CommandQueue queue)
{
}

bool VulkanDevice::PollEventQuery(IEventQuery* query)
{
    return false;
}

void VulkanDevice::WaitEventQuery(IEventQuery* query)
{
}

void VulkanDevice::ResetEventQuery(IEventQuery* query)
{
}

TimerQueryHandle VulkanDevice::CreateTimerQuery()
{
    return TimerQueryHandle();
}

bool VulkanDevice::PollTimerQuery(ITimerQuery* query)
{
    return false;
}

float VulkanDevice::GetTimerQueryTimeSeconds(ITimerQuery* query)
{
    return 0.0f;
}

void VulkanDevice::ResetTimerQuerySeconds(ITimerQuery* query)
{
}

FramebufferHandle VulkanDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    auto* fb = new VulkanFramebuffer(m_Context);
    fb->desc = desc;
    fb->framebufferInfo = FramebufferInfo(desc);

    attachment_vector<vk::AttachmentDescription2> attDescs(desc.colorAttachments.size());
    attachment_vector<vk::AttachmentReference2> colorAttRefs(desc.colorAttachments.size());
    vk::AttachmentReference2 depthAttRef;

    static_vector<vk::ImageView, MAX_RENDER_TARGETS + 1> attImageViews;
    attImageViews.resize(desc.colorAttachments.size());

    u32 numArrayLayers = 0;

    // Color attachment(s) setup.
    for (u32 i = 0; i < u32(desc.colorAttachments.size()); i++)
    {
        const auto& rt = desc.colorAttachments[i];
        auto* t = checked_cast<VulkanTexture*>(rt.texture);

        assert(fb->framebufferInfo.width == t->desc.width >> rt.subresources.baseMipLevel);
        assert(fb->framebufferInfo.height == t->desc.height >> rt.subresources.baseMipLevel);

        const vk::Format attachmentFormat = (rt.format == Format::UNKNOWN ? t->imageInfo.format : ToVkFormat(rt.format));

        attDescs[i] = vk::AttachmentDescription2()
                          .setFormat(attachmentFormat)
                          .setSamples(t->imageInfo.samples)
                          .setLoadOp(vk::AttachmentLoadOp::eLoad)
                          .setStoreOp(vk::AttachmentStoreOp::eStore)
                          .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
                          .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

        colorAttRefs[i] = vk::AttachmentReference2().setAttachment(i).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        auto subresources = rt.subresources.Resolve(t->desc, true);
        auto dimension = GetTextureDimensionForFramebuffer(t->desc.dimension, subresources.numArrayLayers > 1);

        const auto& view = t->GetSubresourceView(subresources, dimension);
        attImageViews[i] = view.imageView;

        fb->resources.push_back(rt.texture);

        if (numArrayLayers)
            assert(numArrayLayers == subresources.numArrayLayers);
        else
            numArrayLayers = subresources.numArrayLayers;
    }

    // Depth attachment setup.
    if (desc.depthAttachment.Valid())
    {
        const auto& att = desc.depthAttachment;

        auto* t = checked_cast<VulkanTexture*>(att.texture);

        assert(fb->framebufferInfo.width == t->desc.width >> att.subresources.baseMipLevel);
        assert(fb->framebufferInfo.height == t->desc.height >> att.subresources.baseMipLevel);

        vk::ImageLayout depthLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        if (desc.depthAttachment.isReadOnly)
            depthLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;

        attDescs.push_back(vk::AttachmentDescription2()
                               .setFormat(t->imageInfo.format)
                               .setSamples(t->imageInfo.samples)
                               .setLoadOp(vk::AttachmentLoadOp::eLoad)
                               .setStoreOp(vk::AttachmentStoreOp::eStore)
                               .setInitialLayout(depthLayout)
                               .setFinalLayout(depthLayout));
        depthAttRef = vk::AttachmentReference2().setAttachment(u32(attDescs.size()) - 1).setLayout(depthLayout);

        auto subresources = att.subresources.Resolve(t->desc, true);
        auto dimension = GetTextureDimensionForFramebuffer(t->desc.dimension, subresources.numArrayLayers > 1);

        const auto& view = t->GetSubresourceView(subresources, dimension);
        attImageViews.push_back(view.imageView);

        fb->resources.push_back(att.texture);

        if (numArrayLayers)
            assert(numArrayLayers == subresources.numArrayLayers);
        else
            numArrayLayers = subresources.numArrayLayers;
    }

    // Subpass setup.
    auto subpass = vk::SubpassDescription2()
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setColorAttachmentCount(u32(desc.colorAttachments.size()))
                       .setPColorAttachments(colorAttRefs.data())
                       .setPDepthStencilAttachment(desc.depthAttachment.Valid() ? &depthAttRef : nullptr);

    // XXX: Variable shading rate setup.
    // if (desc.shadingRateAttachment.Valid())....

    // Create render pass object.
    auto renderPassInfo = vk::RenderPassCreateInfo2()
                              .setAttachmentCount(u32(attDescs.size()))
                              .setPAttachments(attDescs.data())
                              .setSubpassCount(1)
                              .setPSubpasses(&subpass);
    auto res = m_Context.device.createRenderPass2(&renderPassInfo, nullptr, &fb->renderPass);
    VK_CHECK_RETURN_NULL(res);

    // Create framebuffer object.
    auto fbCreateInfo = vk::FramebufferCreateInfo()
                            .setRenderPass(fb->renderPass)
                            .setAttachmentCount(u32(attImageViews.size()))
                            .setPAttachments(attImageViews.data())
                            .setWidth(fb->framebufferInfo.width)
                            .setHeight(fb->framebufferInfo.height)
                            .setLayers(numArrayLayers);

    res = m_Context.device.createFramebuffer(&fbCreateInfo, nullptr, &fb->framebuffer);
    VK_CHECK_RETURN_NULL(res);

    return FramebufferHandle::Create(fb);
}

GraphicsPipelineHandle VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer_)
{
    vk::Result res;

    auto* fb = checked_cast<VulkanFramebuffer*>(framebuffer_);
    auto* inputLayout = checked_cast<VulkanInputLayout*>(desc.inputLayout.Get());

    auto* gp = new VulkanGraphicsPipeline(m_Context);
    gp->desc = desc;
    gp->framebufferInfo = fb->framebufferInfo;

    for (const auto& l : desc.bindingLayouts)
    {
        auto* layout = checked_cast<VulkanBindingLayout*>(l.Get());
        gp->pipelineBindingLayouts.push_back(layout);
    }

    // Shaders setup.
    auto* vertexShader = checked_cast<VulkanShader*>(desc.vertexShader.Get());
    auto* geometryShader = checked_cast<VulkanShader*>(desc.geometryShader.Get());
    auto* fragmentShader = checked_cast<VulkanShader*>(desc.fragmentShader.Get());

    size_t numShaders = 0;
    if (vertexShader)
        numShaders++;
    if (geometryShader)
        numShaders++;
    if (fragmentShader)
        numShaders++;

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(numShaders);

    const auto MakeShaderStageCreateInfo = [](VulkanShader* shader) {
        auto shaderStageCreateInfo = vk::PipelineShaderStageCreateInfo()
                                         .setStage(shader->stageFlagBits)
                                         .setModule(shader->shaderModule)
                                         .setPName(shader->desc.entryName.c_str());

        return shaderStageCreateInfo;
    };

    if (vertexShader)
    {
        shaderStages.push_back(MakeShaderStageCreateInfo(vertexShader));
        gp->shaderMask = gp->shaderMask | ShaderType::VERTEX;
    }

    if (geometryShader)
    {
        shaderStages.push_back(MakeShaderStageCreateInfo(geometryShader));
        gp->shaderMask = gp->shaderMask | ShaderType::GEOMETRY;
    }

    if (fragmentShader)
    {
        shaderStages.push_back(MakeShaderStageCreateInfo(fragmentShader));
        gp->shaderMask = gp->shaderMask | ShaderType::FRAGMENT;
    }

    // Vertex input state setup.
    auto vertexInput = vk::PipelineVertexInputStateCreateInfo();
    if (inputLayout)
    {
        vertexInput.setVertexBindingDescriptionCount(u32(inputLayout->bindingDesc.size()))
            .setPVertexBindingDescriptions(inputLayout->bindingDesc.data())
            .setVertexAttributeDescriptionCount(u32(inputLayout->attributeDesc.size()))
            .setPVertexAttributeDescriptions(inputLayout->attributeDesc.data());
    }
    auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo().setTopology(ToVkPrimitiveTopology(desc.primitiveTopology));

    const auto& rasterState = desc.renderState.rasterizerState;
    const auto& depthStencilState = desc.renderState.depthStencilState;
    const auto& blendState = desc.renderState.blendState;

    auto viewportState = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

    auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
                          .setPolygonMode(ToVkPolygonMode(rasterState.fillMode))
                          .setCullMode(ToVkCullMode(rasterState.cullMode))
                          .setFrontFace((rasterState.frontFaceState == FrontFaceState::COUNTERCLOCKWISE) ? vk::FrontFace::eCounterClockwise
                                                                                                         : vk::FrontFace::eClockwise)
                          .setDepthBiasEnable(rasterState.depthBias ? true : false)
                          .setDepthBiasConstantFactor(float(rasterState.depthBias))
                          .setDepthBiasClamp(rasterState.depthBiasClamp)
                          .setDepthBiasSlopeFactor(rasterState.depthBiasSlopeFactor)
                          .setLineWidth(1.0f);

    auto multisample = vk::PipelineMultisampleStateCreateInfo()
                           .setRasterizationSamples(vk::SampleCountFlagBits(fb->framebufferInfo.sampleCount))
                           .setAlphaToCoverageEnable(blendState.alphaToCoverageEnable);

    auto depthStencil = vk::PipelineDepthStencilStateCreateInfo()
                            .setDepthTestEnable(depthStencilState.depthTestEnable)
                            .setDepthWriteEnable(depthStencilState.depthWriteEnable)
                            .setDepthCompareOp(ToVkCompareOp(depthStencilState.depthFunc))
                            .setStencilTestEnable(depthStencilState.stencilEnable)
                            .setFront(ConvertStencilState(depthStencilState, depthStencilState.frontFaceStencil))
                            .setBack(ConvertStencilState(depthStencilState, depthStencilState.backFaceStencil));

    // XXX: Variable fragment shading rate setup/implementation?

    // Bindings.
    BindingVector<vk::DescriptorSetLayout> descriptorSetLayouts;
    u32 pushConstantSize = 0;
    for (const auto& l : desc.bindingLayouts)
    {
        auto* layout = checked_cast<VulkanBindingLayout*>(l.Get());
        descriptorSetLayouts.push_back(layout->descriptorSetLayout);

        if (!layout->isBindless)
        {
            for (const auto& item : layout->desc.bindings)
            {
                if (item.type == BindingResourceType::PUSH_CONSTANTS)
                {

                    pushConstantSize = item.size;
                    // XXX: Make sure there is only 1 push constant per layout
                    break;
                }
            }
        }
    }

    auto pushConstantRange
        = vk::PushConstantRange().setOffset(0).setSize(pushConstantSize).setStageFlags(ToVkShaderStageFlagBits(gp->shaderMask));

    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
                                  .setSetLayoutCount(u32(descriptorSetLayouts.size()))
                                  .setPSetLayouts(descriptorSetLayouts.data())
                                  .setPushConstantRangeCount(pushConstantSize ? 1 : 0)
                                  .setPPushConstantRanges(&pushConstantRange);

    res = m_Context.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, &gp->pipelineLayout);
    VK_CHECK_RETURN_NULL(res);

    // XXX: Properly handle blending
    // attachment_vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(fb->desc.colorAttachments.size());

    // for (u32 i = 0; i < u32(fb->desc.colorAttachments.size()); i++)
    //{
    //     colorBlendAttachments[i] = convertBlendState(blendState.targets[i]);
    // }

    // auto colorBlend = vk::PipelineColorBlendStateCreateInfo()
    //                       .setAttachmentCount(u32(colorBlendAttachments.size()))
    //                       .setPAttachments(colorBlendAttachments.data());

    // gp->usesBlendConstants = blendState.usesConstantColor(u32(fb->desc.colorAttachments.size()));

    vk::DynamicState dynamicStates[4] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eBlendConstants,
                                         vk::DynamicState::eFragmentShadingRateKHR};

    auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo().setDynamicStateCount(2).setPDynamicStates(dynamicStates);

    auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
                            .setStageCount(u32(shaderStages.size()))
                            .setPStages(shaderStages.data())
                            .setPVertexInputState(&vertexInput)
                            .setPInputAssemblyState(&inputAssembly)
                            .setPViewportState(&viewportState)
                            .setPRasterizationState(&rasterizer)
                            .setPMultisampleState(&multisample)
                            .setPDepthStencilState(&depthStencil)
                            .setPColorBlendState(/* &colorBlend */ nullptr)
                            .setPDynamicState(&dynamicStateInfo)
                            .setLayout(gp->pipelineLayout)
                            .setRenderPass(fb->renderPass)
                            .setSubpass(0)
                            .setBasePipelineHandle(nullptr)
                            .setBasePipelineIndex(-1)
                            .setPTessellationState(nullptr)
                            .setPNext(/*&shadingRateState*/ nullptr);

    auto tessellationState = vk::PipelineTessellationStateCreateInfo();

    // XXX: Handle tesselation.
    // if (desc.primType == PrimitiveType::PatchList)
    //{
    //    tessellationState.setPatchControlPoints(desc.patchControlPoints);
    //    pipelineInfo.setPTessellationState(&tessellationState);
    //}

    res = m_Context.device.createGraphicsPipelines(m_Context.pipelineCache, 1, &pipelineInfo, nullptr, &gp->pipeline);
    VK_CHECK_RETURN_NULL(res);

    return GraphicsPipelineHandle::Create(gp);
}

ComputePipelineHandle VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return nullptr;
}

BindingLayoutHandle VulkanDevice::CreateBindingLayout(const BindingLayoutDesc& desc)
{
    auto* bindingLayout = new VulkanBindingLayout(m_Context, desc);
    bindingLayout->GenerateLayout();

    return BindingLayoutHandle::Create(bindingLayout);
}

BindingLayoutHandle VulkanDevice::CreateBindlessLayout(const BindlessLayoutDesc& desc)
{
    auto* bindingLayout = new VulkanBindingLayout(m_Context, desc);
    bindingLayout->GenerateLayout();

    return BindingLayoutHandle::Create(bindingLayout);
}

BindingSetHandle VulkanDevice::CreateBindingSet(const BindingSetDesc& desc, IBindingLayout* layout_)
{
    auto* layout = checked_cast<VulkanBindingLayout*>(layout_);

    auto* bindingSet = new VulkanBindingSet(m_Context);
    bindingSet->desc = desc;
    bindingSet->layout = layout;

    const auto& descriptorSetLayout = layout->descriptorSetLayout;
    const auto& poolSizes = layout->descriptorPoolSizes;

    auto poolInfo = vk::DescriptorPoolCreateInfo().setPoolSizeCount(u32(poolSizes.size())).setPPoolSizes(poolSizes.data()).setMaxSets(1);

    auto res = m_Context.device.createDescriptorPool(&poolInfo, nullptr, &bindingSet->descriptorPool);
    VK_CHECK_RETURN_NULL(res);

    static_vector<vk::DescriptorImageInfo, C_MAX_BINDING_LAYOUTS> descriptorImageInfo;
    static_vector<vk::DescriptorBufferInfo, C_MAX_BINDING_LAYOUTS> descriptorBufferInfo;
    static_vector<vk::WriteDescriptorSet, C_MAX_BINDING_LAYOUTS> descriptorWriteInfo;

    const auto AddWriteDescriptorData = [&](u32 bindingLocation, vk::DescriptorType descriptorType, vk::DescriptorImageInfo* imageInfo,
                                            vk::DescriptorBufferInfo* bufferInfo, vk::BufferView* bufferView, const void* pNext = nullptr) {
        descriptorWriteInfo.push_back(vk::WriteDescriptorSet()
                                          .setDstSet(bindingSet->descriptorSet)
                                          .setDstBinding(bindingLocation)
                                          .setDstArrayElement(0)
                                          .setDescriptorCount(1)
                                          .setDescriptorType(descriptorType)
                                          .setPImageInfo(imageInfo)
                                          .setPBufferInfo(bufferInfo)
                                          .setPTexelBufferView(bufferView)
                                          .setPNext(pNext));
    };

    for (size_t bindingIndex = 0; bindingIndex < desc.bindings.size(); bindingIndex++)
    {
        const BindingSetItem& binding = desc.bindings[bindingIndex];
        const vk::DescriptorSetLayoutBinding& layoutBinding = layout->descriptorLayoutBindings[bindingIndex];

        if (binding.resourceHandle == nullptr)
        {
            continue;
        }

        // Keep reference to the (binding) resource.
        bindingSet->resources.push_back(binding.resourceHandle);

        switch (binding.type)
        {
        // Texture SRV.
        case BindingResourceType::SAMPLED_IMAGE: {
            const auto texture = checked_cast<VulkanTexture*>(binding.resourceHandle);

            const auto subresource = binding.textureSubresources.Resolve(texture->desc, false);
            const auto textureViewType = ToTextureViewType(binding.format, texture->desc.format);
            auto& view = texture->GetSubresourceView(subresource, binding.dimension, textureViewType);

            auto& imageInfo = descriptorImageInfo.emplace_back();
            imageInfo = vk::DescriptorImageInfo().setImageView(view.imageView).setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

            AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);

            if (!texture->permanentState)
                bindingSet->bindingsInTransitions.push_back(static_cast<u16>(bindingIndex));
        }

        break;

        // Texture UAV.
        case BindingResourceType::STORAGE_IMAGE: {
            const auto texture = checked_cast<VulkanTexture*>(binding.resourceHandle);

            const auto subresource = binding.textureSubresources.Resolve(texture->desc, false);
            const auto textureViewType = ToTextureViewType(binding.format, texture->desc.format);
            auto& view = texture->GetSubresourceView(subresource, binding.dimension, textureViewType);

            auto& imageInfo = descriptorImageInfo.emplace_back();
            imageInfo = vk::DescriptorImageInfo().setImageView(view.imageView).setImageLayout(vk::ImageLayout::eGeneral);

            AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);

            if (!texture->permanentState)
                bindingSet->bindingsInTransitions.push_back(static_cast<u16>(bindingIndex));
        }

        break;

        // Typed buffer SRV and UAV.
        case BindingResourceType::UNIFORM_TEXEL_BUFFER:
        case BindingResourceType::STORAGE_TEXEL_BUFFER: {
            const auto buffer = checked_cast<VulkanBuffer*>(binding.resourceHandle);

            const bool isStorage = (binding.type == BindingResourceType::STORAGE_TEXEL_BUFFER);

            Format format = binding.format;

            if (format == Format::UNKNOWN)
            {
                format = buffer->desc.format;
            }

            auto vkformat = ToVkFormat(format);

            const auto& bufferViewFound = buffer->viewCache.find(vkformat);
            auto& bufferViewRef = (bufferViewFound != buffer->viewCache.end()) ? bufferViewFound->second : buffer->viewCache[vkformat];
            if (bufferViewFound == buffer->viewCache.end())
            {
                assert(format != Format::UNKNOWN);
                const auto range = binding.bufferRange.Resolve(buffer->desc);

                auto bufferViewInfo = vk::BufferViewCreateInfo()
                                          .setBuffer(buffer->buffer)
                                          .setOffset(range.byteOffset)
                                          .setRange(range.byteSize)
                                          .setFormat(vkformat);

                res = m_Context.device.createBufferView(&bufferViewInfo, nullptr, &bufferViewRef);
                VK_ASSERT_OK(res);
            }

            AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, nullptr, nullptr, &bufferViewRef);

            if (!buffer->permanentState)
                bindingSet->bindingsInTransitions.push_back(static_cast<u16>(bindingIndex));
        }
        break;

        // Normal UAV/SRV buffers.
        case BindingResourceType::STORAGE_BUFFER:
        case BindingResourceType::UNIFORM_BUFFER:
        case BindingResourceType::UNIFORM_BUFFER_DYNAMIC: {
            const auto buffer = checked_cast<VulkanBuffer*>(binding.resourceHandle);

            const auto range = binding.bufferRange.Resolve(buffer->desc);

            auto& bufferInfo = descriptorBufferInfo.emplace_back();
            bufferInfo = vk::DescriptorBufferInfo().setBuffer(buffer->buffer).setOffset(range.byteOffset).setRange(range.byteSize);

            assert(buffer->buffer);
            AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, nullptr, &bufferInfo, nullptr);

            if (binding.type == BindingResourceType::UNIFORM_BUFFER_DYNAMIC)
            {
                // XXX: Handle volatile/dynamic buffers.
            }
            else
            {
                if (!buffer->permanentState)
                    bindingSet->bindingsInTransitions.push_back(static_cast<u16>(bindingIndex));
                else
                {
                    // ResourceStates requiredState;
                    // if (binding.type == BindingResourceType::STORAGE_BUFFER)
                    //     requiredState = ResourceStates::UNORDERED_ACCESS;
                    // else if (binding.type == BindingResourceType::UNIFORM_BUFFER)
                    //     requiredState = ResourceStates::CONSTANT_BUFFER;
                    // else
                    //     requiredState = ResourceStates::SHADER_RESOURCE;

                    // XXX: Verify required state?
                }
            }
        }

        break;

        case BindingResourceType::SAMPLER: {
            const auto sampler = checked_cast<VulkanSampler*>(binding.resourceHandle);

            auto& imageInfo = descriptorImageInfo.emplace_back();
            imageInfo = vk::DescriptorImageInfo().setSampler(sampler->sampler);

            AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);
        }

        break;

        case BindingResourceType::PUSH_CONSTANTS:
            break;
        default:
            LOG_ERROR("CreateBindingSet: Invalid enum!");
            break;
        }
    }

    m_Context.device.updateDescriptorSets(u32(descriptorWriteInfo.size()), descriptorWriteInfo.data(), 0, nullptr);

    return BindingSetHandle::Create(bindingSet);
}

DescriptorTableHandle VulkanDevice::CreateDescriptorTable(IBindingLayout* layout_)
{
    auto* layout = checked_cast<VulkanBindingLayout*>(layout_);

    auto* descriptorTable = new VulkanDescriptorTable(m_Context);
    descriptorTable->layout = layout;
    descriptorTable->capacity = layout->descriptorLayoutBindings[0].descriptorCount;

    const auto& descriptorSetLayout = layout->descriptorSetLayout;
    const auto& poolSizes = layout->descriptorPoolSizes;

    auto poolInfo = vk::DescriptorPoolCreateInfo().setPoolSizeCount(u32(poolSizes.size())).setPPoolSizes(poolSizes.data()).setMaxSets(1);

    // Create descriptor pool.
    auto res = m_Context.device.createDescriptorPool(&poolInfo, nullptr, &descriptorTable->descriptorPool);
    VK_CHECK_RETURN_NULL(res);

    // Allocate descriptor set.
    auto descriptorSetAllocInfo = vk::DescriptorSetAllocateInfo()
                                      .setDescriptorPool(descriptorTable->descriptorPool)
                                      .setDescriptorSetCount(1)
                                      .setPSetLayouts(&descriptorSetLayout);

    res = m_Context.device.allocateDescriptorSets(&descriptorSetAllocInfo, &descriptorTable->descriptorSet);
    VK_CHECK_RETURN_NULL(res);

    return DescriptorTableHandle();
}

void VulkanDevice::ResizeDescriptorTable(IDescriptorTable* descriptorTable_, u32 newSize, bool keepContents)
{
    // XXX: Do nothing for now? Descriptor table is always created with 'max capacity'.
    auto descriptorTable = checked_cast<VulkanDescriptorTable*>(descriptorTable_);
    assert(newSize <= descriptorTable->layout->GetBindlessDesc()->maxCapacity);
}

bool VulkanDevice::WriteDescriptorTable(IDescriptorTable* descriptorTable_, const BindingSetItem& binding)
{
    auto* descriptorTable = checked_cast<VulkanDescriptorTable*>(descriptorTable_);
    auto* layout = checked_cast<VulkanBindingLayout*>(descriptorTable->layout.Get());

    static_vector<vk::DescriptorImageInfo, C_MAX_BINDING_LAYOUTS> descriptorImageInfo;
    static_vector<vk::DescriptorBufferInfo, C_MAX_BINDING_LAYOUTS> descriptorBufferInfo;
    static_vector<vk::WriteDescriptorSet, C_MAX_BINDING_LAYOUTS> descriptorWriteInfo;

    const auto AddWriteDescriptorData = [&](u32 bindingLocation, vk::DescriptorType descriptorType, vk::DescriptorImageInfo* imageInfo,
                                            vk::DescriptorBufferInfo* bufferInfo, vk::BufferView* bufferView, const void* pNext = nullptr) {
        descriptorWriteInfo.push_back(vk::WriteDescriptorSet()
                                          .setDstSet(descriptorTable->descriptorSet)
                                          .setDstBinding(bindingLocation)
                                          .setDstArrayElement(0)
                                          .setDescriptorCount(1)
                                          .setDescriptorType(descriptorType)
                                          .setPImageInfo(imageInfo)
                                          .setPBufferInfo(bufferInfo)
                                          .setPTexelBufferView(bufferView)
                                          .setPNext(pNext));
    };

    vk::Result res;

    for (u32 bindingLocation = 0; bindingLocation < u32(layout->bindlessDesc.registerSpaces.size()); bindingLocation++)
    {
        if (layout->bindlessDesc.registerSpaces[bindingLocation].type == binding.type)
        {
            const vk::DescriptorSetLayoutBinding& layoutBinding = layout->descriptorLayoutBindings[bindingLocation];

            switch (binding.type)
            {
            case BindingResourceType::SAMPLED_IMAGE: {
                const auto& texture = checked_cast<VulkanTexture*>(binding.resourceHandle);

                const auto subresource = binding.textureSubresources.Resolve(texture->desc, false);
                const auto textureViewType = ToTextureViewType(binding.format, texture->desc.format);
                auto& view = texture->GetSubresourceView(subresource, binding.dimension, textureViewType);

                auto& imageInfo = descriptorImageInfo.emplace_back();
                imageInfo = vk::DescriptorImageInfo().setImageView(view.imageView).setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

                AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);
            }

            break;

            case BindingResourceType::STORAGE_IMAGE: {
                const auto texture = checked_cast<VulkanTexture*>(binding.resourceHandle);

                const auto subresource = binding.textureSubresources.Resolve(texture->desc, true);
                const auto textureViewType = ToTextureViewType(binding.format, texture->desc.format);
                auto& view = texture->GetSubresourceView(subresource, binding.dimension, textureViewType);

                auto& imageInfo = descriptorImageInfo.emplace_back();
                imageInfo = vk::DescriptorImageInfo().setImageView(view.imageView).setImageLayout(vk::ImageLayout::eGeneral);

                AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);
            }

            break;

            case BindingResourceType::UNIFORM_TEXEL_BUFFER:
            case BindingResourceType::STORAGE_TEXEL_BUFFER: {
                const auto& buffer = checked_cast<VulkanBuffer*>(binding.resourceHandle);

                auto vkformat = ToVkFormat(binding.format);

                const auto& bufferViewFound = buffer->viewCache.find(vkformat);
                auto& bufferViewRef = (bufferViewFound != buffer->viewCache.end()) ? bufferViewFound->second : buffer->viewCache[vkformat];
                if (bufferViewFound == buffer->viewCache.end())
                {
                    assert(binding.format != Format::UNKNOWN);
                    const auto range = binding.bufferRange.Resolve(buffer->desc);

                    auto bufferViewInfo = vk::BufferViewCreateInfo()
                                              .setBuffer(buffer->buffer)
                                              .setOffset(range.byteOffset)
                                              .setRange(range.byteSize)
                                              .setFormat(vkformat);

                    res = m_Context.device.createBufferView(&bufferViewInfo, nullptr, &bufferViewRef);
                    VK_ASSERT_OK(res);
                }

                AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, nullptr, nullptr, &bufferViewRef);
            }
            break;

            // Normal UAV/storage or uniform or SRV buffers.
            case BindingResourceType::STORAGE_BUFFER:
            case BindingResourceType::UNIFORM_BUFFER:
            case BindingResourceType::UNIFORM_BUFFER_DYNAMIC: {
                const auto buffer = checked_cast<VulkanBuffer*>(binding.resourceHandle);

                const auto range = binding.bufferRange.Resolve(buffer->desc);

                auto& bufferInfo = descriptorBufferInfo.emplace_back();
                bufferInfo = vk::DescriptorBufferInfo().setBuffer(buffer->buffer).setOffset(range.byteOffset).setRange(range.byteSize);

                assert(buffer->buffer);
                AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, nullptr, &bufferInfo, nullptr);
            }

            break;

            case BindingResourceType::SAMPLER: {
                const auto& sampler = checked_cast<VulkanSampler*>(binding.resourceHandle);

                auto& imageInfo = descriptorImageInfo.emplace_back();
                imageInfo = vk::DescriptorImageInfo().setSampler(sampler->sampler);

                AddWriteDescriptorData(layoutBinding.binding, layoutBinding.descriptorType, &imageInfo, nullptr, nullptr);
            }

            break;

            case BindingResourceType::PUSH_CONSTANTS:
                break;

            case BindingResourceType::NONE:
            case BindingResourceType::COUNT:
            default:
                LOG_ERROR("WriteDescriptorTable: Invalid enum!");
            }
        }
    }

    m_Context.device.updateDescriptorSets(u32(descriptorWriteInfo.size()), descriptorWriteInfo.data(), 0, nullptr);

    return true;
}

CommandListHandle VulkanDevice::CreateCommandList(const CommandListParameters& params)
{
    if (!m_Queues[u32(params.queueType)])
        return nullptr;

    VulkanCommandList* commandList = new VulkanCommandList(this, m_Context, params);

    return CommandListHandle::Create(commandList);
}

u64 VulkanDevice::ExecuteCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists, CommandQueue executionQueue)
{
    assert(m_Queues[u32(executionQueue)] != nullptr);

    auto& q = *m_Queues[u32(executionQueue)];

    auto submissionID = q.Submit(pCommandLists, numCommandLists);

    for (size_t i = 0; i < numCommandLists; i++)
    {
        checked_cast<VulkanCommandList*>(pCommandLists[i])->MarkExecuted(q, submissionID);
    }

    return submissionID;
}

void VulkanDevice::QueueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, u64 instance)
{
    QueueWaitForSemaphore(waitQueue, GetQueueSemaphore(executionQueue), instance);
}

void VulkanDevice::WaitForIdle()
{
    m_Context.device.waitIdle();
}

void VulkanDevice::Present(ITexture* texture)
{
}

void VulkanDevice::Cleanup()
{
    for (auto& q : m_Queues)
    {
        if (q)
        {
            q->RetireCommandBuffers();
        }
    }
}

Object VulkanDevice::GetNativeQueue(ObjectType objectType, CommandQueue queue)
{
    if (objectType != ObjectTypes::Vk_Queue)
        return Object(nullptr);

    if (queue >= CommandQueue::COUNT)
        return Object(nullptr);

    return Object(m_Queues[u32(queue)]->GetVkQueue());
}

bool VulkanDevice::QueryFeatureSupport(RHIFeature feature, void* pInfo, size_t infoSize)
{
    // Do nothing for now.
    return false;
}

FormatSupport VulkanDevice::QueryFormatSupport(Format format)
{
    vk::Format vulkanFormat = ToVkFormat(format);
    auto props = m_Context.physicalDevice.getFormatProperties(vulkanFormat);

    FormatSupport result = FormatSupport::NONE;

    if (props.bufferFeatures)
        result = result | FormatSupport::BUFFER;
    if (format == Format::R32_UINT || format == Format::R16_UINT)
        result = result | FormatSupport::INDEX_BUFFER;
    if (props.bufferFeatures & vk::FormatFeatureFlagBits::eVertexBuffer)
        result = result | FormatSupport::VERTEX_BUFFER;
    if (props.optimalTilingFeatures)
        result = result | FormatSupport::TEXTURE;
    if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        result = result | FormatSupport::DEPTH_STENCIL;
    if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
        result = result | FormatSupport::RENDER_TARGET;
    if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachmentBlend)
        result = result | FormatSupport::BLENDABLE;
    if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage)
        || (props.bufferFeatures & vk::FormatFeatureFlagBits::eUniformTexelBuffer))
        result = result | FormatSupport::SHADER_LOAD;
    if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)
        result = result | FormatSupport::SHADER_SAMPLE;
    if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImage)
        || (props.bufferFeatures & vk::FormatFeatureFlagBits::eStorageTexelBuffer))
    {
        result = result | FormatSupport::SHADER_UAV_LOAD;
        result = result | FormatSupport::SHADER_UAV_STORE;
    }
    if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eStorageImageAtomic)
        || (props.bufferFeatures & vk::FormatFeatureFlagBits::eStorageTexelBufferAtomic))
        result = result | FormatSupport::SHADER_ATOMIC;

    return result;
}

vk::Semaphore VulkanDevice::GetQueueSemaphore(CommandQueue queue)
{
    auto& q = *m_Queues[u32(queue)];

    return q.trackingSemaphore;
}

void VulkanDevice::QueueWaitForSemaphore(CommandQueue waitQueue, vk::Semaphore semaphore, u64 value)
{
    auto& q = *m_Queues[u32(waitQueue)];
    q.AddWaitSemaphore(semaphore, value);
}

void VulkanDevice::QueueSignalSemaphore(CommandQueue executionQueue, vk::Semaphore semaphore, u64 value)
{
    auto& q = *m_Queues[u32(executionQueue)];
    q.AddSignalSemaphore(semaphore, value);
}

u64 VulkanDevice::QueueGetCompletedInstance(CommandQueue queue)
{
    return m_Context.device.getSemaphoreCounterValue(GetQueueSemaphore(queue));
}

FramebufferHandle VulkanDevice::CreateHandleForNativeFramebuffer(vk::RenderPass renderPass, vk::Framebuffer framebuffer,
                                                                 const FramebufferDesc& desc, bool transferOwnership)
{
    auto* fb = new VulkanFramebuffer(m_Context);
    fb->desc = desc;
    fb->framebufferInfo = FramebufferInfo(desc);
    fb->renderPass = renderPass;
    fb->framebuffer = framebuffer;
    fb->managed = transferOwnership;

    for (const auto& rt : desc.colorAttachments)
    {
        if (rt.Valid())
            fb->resources.push_back(rt.texture);
    }

    if (desc.depthAttachment.Valid())
    {
        fb->resources.push_back(desc.depthAttachment.texture);
    }

    return FramebufferHandle::Create(fb);
}

void* VulkanDevice::MapBuffer(IBuffer* b, CpuAccessMode flags, uint64_t offset, size_t size) const
{
    auto* buffer = checked_cast<VulkanBuffer*>(b);

    // XXX: Do we need these? vma handles these for us.
    assert(flags != CpuAccessMode::NONE);
    vk::AccessFlags accessFlags;
    switch (flags)
    {
    case CpuAccessMode::READ:
        accessFlags = vk::AccessFlagBits::eHostRead;
        break;
    case CpuAccessMode::WRITE:
        accessFlags = vk::AccessFlagBits::eHostWrite;
        break;
    case CpuAccessMode::NONE:
    default:
        break;
    }

    // XXX: rhere should be a barrier? There can't be a command list here however.
    // buffer->barrier(cmd, vk::PipelineStageFlagBits::eHost, accessFlags);

    void* ptr = nullptr;
    vmaMapMemory(m_Context.allocator, buffer->allocation, &ptr);
    ptr = (char*)ptr + offset;

    return ptr;
}

void VulkanDevice::InitAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_Context.physicalDevice;
    allocatorInfo.device = m_Context.device;
    allocatorInfo.instance = m_Context.instance;

    vmaCreateAllocator(&allocatorInfo, &m_Context.allocator);
}

} // namespace Vulkan

} // namespace SuohRHI