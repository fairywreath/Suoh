#include "VulkanDevice.h"

#include "VulkanUtils.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace RenderLib
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
    InitSwapchainImages();
    InitSynchronizationObjects();

    glslang_initialize_process();
}

VulkanDevice::~VulkanDevice()
{
    WaitIdle();

    glslang_finalize_process();

    m_Context.device.destroySemaphore(m_GraphicsSubmissionSemaphore);
    m_Context.device.destroySemaphore(m_RenderSemaphore);

    vmaDestroyAllocator(m_Context.allocator);

    m_Swapchain.Destroy();
    m_Context.device.destroy();
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

void VulkanDevice::InitSwapchainImages()
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

void VulkanDevice::InitSynchronizationObjects()
{
    const auto timelineInfo = vk::SemaphoreTypeCreateInfo()
                                  .setSemaphoreType(vk::SemaphoreType::eTimeline)
                                  .setInitialValue(0);

    const auto semaphoreInfo = vk::SemaphoreCreateInfo().setPNext(&timelineInfo);

    auto res
        = m_Context.device.createSemaphore(&semaphoreInfo, nullptr, &m_GraphicsSubmissionSemaphore);
    if (res != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create graphics queue timeline semaphore!");
    }

    // Create render semaphore.
    const auto renderSemaphoreInfo = vk::SemaphoreCreateInfo();
    res = m_Context.device.createSemaphore(&renderSemaphoreInfo, nullptr, &m_RenderSemaphore);
    if (res != vk::Result::eSuccess)
    {
        LOG_ERROR("Failed to create binary render semaphore!");
    }
}

void VulkanDevice::Submit(CommandList* commandList)
{
    WaitGraphicsSubmissionSemaphore();

    m_LastSubmittedGraphicsID++;

    std::vector<vk::Semaphore> waitSemaphores;
    std::vector<vk::PipelineStageFlags> waitStages;

    std::vector<vk::Semaphore> signalSemaphores{m_GraphicsSubmissionSemaphore};
    std::vector<u64> signalSemaphoreValues{m_LastSubmittedGraphicsID};

    if (commandList->desc.usage == CommandListUsage::GRAPHICS)
    {
        // Wait for swapchain acquire next image.
        waitSemaphores.push_back(m_Swapchain.GetCurrentPresentSemaphore());

        waitStages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);

        // Set ignal swapchain present semaphore.
        signalSemaphores.push_back(m_RenderSemaphore);

        // Add binary render semaphore value count(although ignored).
        signalSemaphoreValues.push_back(0);
    }

    auto timelineInfo
        = vk::TimelineSemaphoreSubmitInfo().setSignalSemaphoreValues(signalSemaphoreValues);

    const auto submitInfo = vk::SubmitInfo()
                                //.setWaitSemaphores(m_Swapchain.GetCurrentPresentSemaphore())
                                //.setWaitDstStageMask(waitStages)
                                .setCommandBuffers(commandList->GetCommandBuffer())
                                .setSignalSemaphores(signalSemaphores)
                                .setPNext(&timelineInfo);

    VK_CHECK_RETURN(m_GraphicsQueue.submit(1, &submitInfo, nullptr));
}

void VulkanDevice::WaitGraphicsSubmissionSemaphore()
{
    const auto waitInfo = vk::SemaphoreWaitInfo()
                              .setSemaphores(m_GraphicsSubmissionSemaphore)
                              .setValues(m_LastSubmittedGraphicsID);
    VK_CHECK_RETURN(m_Context.device.waitSemaphores(&waitInfo, 1000000000));

    m_LastFinishedGraphicsID
        = m_Context.device.getSemaphoreCounterValue(m_GraphicsSubmissionSemaphore);

    assert(m_LastFinishedGraphicsID == m_LastSubmittedGraphicsID);
}

void VulkanDevice::WaitTransferSubmissionSemaphore()
{
    const auto waitInfo = vk::SemaphoreWaitInfo()
                              .setSemaphores(m_TransferSubmissionSemaphore)
                              .setValues(m_LastSubmittedTransferID);
    VK_CHECK_RETURN(m_Context.device.waitSemaphores(&waitInfo, 1000000000));

    m_LastFinishedTransferID
        = m_Context.device.getSemaphoreCounterValue(m_TransferSubmissionSemaphore);

    assert(m_LastFinishedTransferID == m_LastSubmittedTransferID);
}

void VulkanDevice::Present()
{
    m_Swapchain.Present(m_RenderSemaphore);
}

void VulkanDevice::WaitIdle()
{
    m_Context.device.waitIdle();
}

void VulkanDevice::SwapchainAcquireNextImage()
{
    m_Swapchain.AcquireNextImage();
}

} // namespace Vulkan

} // namespace RenderLib
