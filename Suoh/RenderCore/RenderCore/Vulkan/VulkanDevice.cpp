#include "VulkanDevice.h"

#include "VulkanUtils.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace RenderCore
{

namespace Vulkan
{

VulkanDevice::VulkanDevice(const DeviceDesc& desc) : m_Instance(desc)
{
    const auto deviceContext = m_Instance.CreateDevice(desc);

    m_GraphicsQueue = deviceContext.graphicsQueue;
    m_GraphicsFamily = deviceContext.graphicsFamily;
    m_PresentQueue = deviceContext.presentQueue;
    m_PresentFamily = deviceContext.presentFamily;
    m_ComputeQueue = deviceContext.computeQueue;
    m_ComputeFamily = deviceContext.computeFamily;

    m_Context.device = deviceContext.device;
    m_Context.physicalDevice = deviceContext.physicalDevice;
    m_Context.instance = m_Instance.GetVkInstance();
    m_Context.surface = m_Instance.GetVkSurfaceKHR();

    m_Swapchain.InitSwapchain(this, m_Context, desc.framebufferWidth, desc.framebufferHeight);

    InitAllocator();

    glslang_initialize_process();
}

VulkanDevice::~VulkanDevice()
{
    glslang_finalize_process();

    vmaDestroyAllocator(m_Context.allocator);

    m_Swapchain.Destroy();
    m_Context.device.destroy();
}

BufferHandle VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    auto buffer = BufferHandle::Create(new Buffer(m_Context));

    vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eTransferSrc
                                       | vk::BufferUsageFlagBits::eTransferDst
                                       | ToVkBufferUsageFlags(desc.usage);

    auto bufferInfo = vk::BufferCreateInfo()
                          .setUsage(bufferUsage)
                          .setSize(desc.size)
                          .setSharingMode(vk::SharingMode::eExclusive)
                          .setPNext(0);

    VmaAllocationCreateInfo allocInfo{};
    if (desc.usage == BufferUsage::UNIFORM_BUFFER || desc.cpuAccess == CpuAccessMode::WRITE)
    {
        allocInfo.flags
            = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else
    {
        allocInfo.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    auto res = vmaCreateBuffer(
        m_Context.allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &allocInfo,
        reinterpret_cast<VkBuffer*>(&buffer->buffer), &buffer->allocation, nullptr);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create buffer!");
        return nullptr;
    }

    buffer->desc = desc;

    return buffer;
}

ImageHandle VulkanDevice::CreateImage(const ImageDesc& desc)
{
    auto image = ImageHandle::Create(new Image(m_Context));

    auto usage = desc.usage | vk::ImageUsageFlagBits::eTransferDst;

    // Create image.
    auto imageInfo
        = vk::ImageCreateInfo()
              .setFlags(desc.flags)
              .setImageType(vk::ImageType::e2D)
              .setFormat(desc.format)
              .setExtent(vk::Extent3D().setWidth(desc.width).setHeight(desc.height).setDepth(1))
              .setMipLevels(1)
              .setArrayLayers((desc.flags & vk::ImageCreateFlagBits::eCubeCompatible) ? 6 : 1)
              .setSamples(vk::SampleCountFlagBits::e1)
              .setTiling(desc.tiling)
              .setUsage(usage)
              .setSharingMode(vk::SharingMode::eExclusive)
              .setInitialLayout(vk::ImageLayout::eUndefined);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    auto res = vmaCreateImage(m_Context.allocator, reinterpret_cast<VkImageCreateInfo*>(&imageInfo),
                              &allocInfo, reinterpret_cast<VkImage*>(&image->image),
                              &image->allocation, nullptr);
    if (res != VK_SUCCESS)
    {
        LOG_ERROR("vmaCreateImage failed!");
        return nullptr;
    }

    image->desc = desc;
    image->desc.usage = usage;
    image->currentLayout = vk::ImageLayout::eUndefined;

    return image;
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    auto sampler = SamplerHandle::Create(new Sampler(m_Context));

    sampler->samplerInfo = vk::SamplerCreateInfo()
                               .setMagFilter(desc.magFilter)
                               .setMinFilter(desc.minFilter)
                               .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                               .setAddressModeU(desc.addressMode)
                               .setAddressModeV(desc.addressMode)
                               .setAddressModeW(desc.addressMode)
                               .setMipLodBias(1.0f)
                               .setAnisotropyEnable(false)
                               .setMaxAnisotropy(1)
                               .setCompareEnable(false)
                               .setCompareOp(vk::CompareOp::eAlways)
                               .setMinLod(0.0f)
                               .setMaxLod(0.0f)
                               .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                               .setUnnormalizedCoordinates(false);

    VK_CHECK_RETURN_NULL(
        m_Context.device.createSampler(&sampler->samplerInfo, nullptr, &sampler->sampler));

    return sampler;
}

ShaderHandle VulkanDevice::CreateShader(const ShaderDesc& _desc)
{
    auto handle = ShaderHandle::Create(new Shader(m_Context));
    ShaderDesc desc = _desc;

    if (desc.readFile)
    {
        if (desc.needsCompilation)
        {
            desc.size = CompileShaderFile(desc.fileName, desc.binary);

            // Save shader binary to file.
            std::string binaryPath = desc.fileName + ".spv";
            SaveShaderBinaryFile(binaryPath, desc.binary, desc.size);
        }
        else
        {
            desc.size = ReadShaderBinaryFile(desc.fileName, desc.binary);
        }
    }

    if (desc.size == 0)
        return nullptr;

    const auto shaderInfo = vk::ShaderModuleCreateInfo()
                                .setCodeSize(desc.binary.size() * sizeof(u32))
                                .setPCode(desc.binary.data());
    VK_CHECK_RETURN_NULL(
        m_Context.device.createShaderModule(&shaderInfo, nullptr, &handle->shaderModule));

    handle->desc = desc;
    return handle;
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

CommandListHandle VulkanDevice::CreateCommandList(const CommandListDesc& desc)
{
    return CommandListHandle::Create(new CommandList(m_Context, this, desc));
}

void* VulkanDevice::MapBuffer(Buffer* buffer)
{
    void* ptr;
    vmaMapMemory(m_Context.allocator, buffer->allocation, &ptr);
    return ptr;
}

void VulkanDevice::UnmapBuffer(Buffer* buffer)
{
    vmaUnmapMemory(m_Context.allocator, buffer->allocation);
}

vk::Format VulkanDevice::FindSupportedFormat(const std::vector<vk::Format>& formats,
                                             vk::ImageTiling tiling,
                                             vk::FormatFeatureFlags features)
{
    bool isLinear = (tiling == vk::ImageTiling::eLinear);
    bool isOptimal = (tiling == vk::ImageTiling::eOptimal);

    for (auto format : formats)
    {
        auto props = m_Context.physicalDevice.getFormatProperties(format);

        if (isLinear && ((props.linearTilingFeatures & features) == features))
        {
            return format;
        }

        if (isOptimal && ((props.optimalTilingFeatures & features) == features))
        {
            return format;
        }
    }

    return vk::Format::eUndefined;
}

vk::Format VulkanDevice::FindDepthFormat()
{
    return FindSupportedFormat(
        {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint,
        },
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void VulkanDevice::InitAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_Context.physicalDevice;
    allocatorInfo.device = m_Context.device;
    allocatorInfo.instance = m_Context.instance;

    vmaCreateAllocator(&allocatorInfo, &m_Context.allocator);
}

void VulkanDevice::InitSwapchainImage()
{
    const auto swapchainImageCount = m_Swapchain.GetImageCount();
    m_SwapchainImages.resize(swapchainImageCount);

    for (int i = 0; i < swapchainImageCount; i++)
    {
        m_SwapchainImages[i] = ImageHandle::Create(new Image(m_Context));
        auto image = m_SwapchainImages[i];

        image->image = m_Swapchain.GetImage(i);
        image->imageView = m_Swapchain.GetImageView(i);
        image->managed = false;
    }
}

} // namespace Vulkan

} // namespace RenderCore
