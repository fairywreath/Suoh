#include "VulkanDevice.h"
#include "GPUConstants.h"
#include "VulkanCommon.h"

#include <Core/Logger.h>

// Default disptach dynamic loader for vulkan.hpp.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

DeferredDeletionQueue::DeferredDeletionQueue(VulkanDevice* device) : m_Device(device)
{
    m_Entries.reserve(4096);
}

DeferredDeletionQueue::~DeferredDeletionQueue()
{
    DestroyResources();
}

void DeferredDeletionQueue::EnqueueVulkanHandle(Type type, u64 handle)
{
    assert(!(type == Type::BufferAllocated) || (type == Type::TextureAllocated));

    Entry entry;
    entry.type = type;
    entry.vulkanHandle = handle;

    m_Entries.push_back(entry);
}

void DeferredDeletionQueue::EnqueueVulkanAllocation(Type type, u64 handle, VmaAllocation allocation)
{
    assert((type == Type::BufferAllocated) || (type == Type::TextureAllocated));

    Entry entry;
    entry.type = type;
    entry.vulkanHandle = handle;
    entry.allocation = allocation;

    m_Entries.push_back(entry);
}

void DeferredDeletionQueue::DestroyResources()
{
    const auto vulkanDevice = m_Device->GetVulkanDevice();
    const auto vmaAllocator = m_Device->GetVmaAllocator();
    const auto allocationCallbacks = m_Device->GetVulkanAllocationCallbacks();

    // XXX TODO: Properly check frame number to ensure object is deleted after frame finishes execution.
    for (const auto& entry : m_Entries)
    {
        switch (entry.type)
        {
        case Type::BufferAllocated:
            vmaDestroyBuffer(vmaAllocator, (VkBuffer)entry.vulkanHandle, entry.allocation);
            break;
        case Type::TextureAllocated:
            vmaDestroyImage(vmaAllocator, (VkImage)entry.vulkanHandle, entry.allocation);
            break;
        case Type::TextureView:
            vulkanDevice.destroyImageView((VkImageView)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::Sampler:
            vulkanDevice.destroySampler((VkSampler)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::ShaderModule:
            vulkanDevice.destroyShaderModule((VkShaderModule)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::RenderPass:
            vulkanDevice.destroyRenderPass((VkRenderPass)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::Framebuffer:
            vulkanDevice.destroyFramebuffer((VkFramebuffer)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::Pipeline:
            vulkanDevice.destroyPipeline((VkPipeline)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::PipelineLayout:
            vulkanDevice.destroyPipelineLayout((VkPipelineLayout)entry.vulkanHandle, allocationCallbacks);
            break;
        case Type::DescriptorSetLayout:
            vulkanDevice.destroyDescriptorSetLayout((VkDescriptorSetLayout)entry.vulkanHandle, allocationCallbacks);
            break;
        default:
            LOG_ERROR("DeferredDeletionQueue::DestroyResources: Unknown entry type!");
            assert(false);
            break;
        }
    }

    m_Entries.clear();
}

#if defined(VULKAN_DEBUG)
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                   void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_ERROR("<Vulkan Validation Layer> ", pCallbackData->pMessage);
    }

    return VK_FALSE;
}
#endif

VulkanDevice::VulkanDevice(const VulkanDeviceDesc& desc) : m_DeferredDeletionQueue(this)
{
    Init(desc);
}

VulkanDevice::~VulkanDevice()
{
    Destroy();
}

void VulkanDevice::Init(const VulkanDeviceDesc& desc)
{
    m_DeviceDesc = desc;

    InitVulkanInstance();
    InitVulkanDebugUtils();
    InitVulkanPhysicalDevice(desc);
    InitVulkanLogicalDevice();
    InitVulkanAllocator();
    InitVulkanGlobalDescriptorPools();
    InitVulkanSynchronizationStructures();
    InitThreadFramePools(desc.numThreads, desc.gpuTimeQueriesPerFrame);

    auto swapchainDesc = SwapchainDesc().SetWindow(desc.window);
    m_Swapchain.Init(this, swapchainDesc);

    m_CommandBufferManager.Init(this, desc.numThreads);

    m_QueuedCommandBuffers.reserve(32);
    m_DescriptorSetUpdates.reserve(32);
    m_BindlessTextureUpdates.reserve(256);
}

void VulkanDevice::Destroy()
{
    m_VulkanDevice.waitIdle();

    m_DeferredDeletionQueue.DestroyResources();

    m_CommandBufferManager.Destroy();

    m_Swapchain.Destroy();

    m_VulkanDevice.destroySemaphore(m_VulkanSemaphoreCompute);
    m_VulkanDevice.destroySemaphore(m_VulkanSemaphoreGraphics);
    m_VulkanDevice.destroySemaphore(m_VulkanSemaphoreImageAcquired);
    for (auto& semaphore : m_VulkanSemaphoresRenderComplete)
    {
        m_VulkanDevice.destroySemaphore(semaphore);
    }

    for (auto& pool : m_ThreadFramePools)
    {
        m_VulkanDevice.destroyCommandPool(pool.commandPool);
        m_VulkanDevice.destroyQueryPool(pool.timestampQueryPool);
        m_VulkanDevice.destroyQueryPool(pool.pipelineStatsQuerypool);
    }

    if (m_BindlessSupported)
    {
        m_VulkanDevice.destroyDescriptorPool(m_VulkanGlobalDescriptorPoolBindless);
    }
    m_VulkanDevice.destroyDescriptorPool(m_VulkanGlobalDescriptorPool);

    vmaDestroyAllocator(m_VmaAllocator);

    m_VulkanDevice.destroy();
    m_VulkanInstance.destroyDebugUtilsMessengerEXT(m_VulkanDebugUtilsMessenger);
    m_VulkanInstance.destroy();
}

void VulkanDevice::InitVulkanInstance()
{
    m_VulkanAllocationCallbacks = nullptr;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr
        = m_VulkanDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    auto appInfo = vk::ApplicationInfo().setApiVersion(VK_API_VERSION_1_3);
    auto instanceInfo = vk::InstanceCreateInfo()
                            .setPApplicationInfo(&appInfo)
                            .setEnabledExtensionCount(S_RequestedVulkanInstanceExtensions.size())
                            .setPpEnabledExtensionNames(S_RequestedVulkanInstanceExtensions.data())
                            .setEnabledLayerCount(S_ReqeustedVulkanInstanceLayers.size())
                            .setPpEnabledLayerNames(S_ReqeustedVulkanInstanceLayers.data());

    // XXX: Properly check layers are supported.
#if defined(VULKAN_DEBUG)
    const vk::ValidationFeatureEnableEXT enabledValidationFeatures[] = {
        vk::ValidationFeatureEnableEXT::eGpuAssisted,
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
        vk::ValidationFeatureEnableEXT::eBestPractices,
    };
    const auto validationFeatures = vk::ValidationFeaturesEXT().setEnabledValidationFeatures(enabledValidationFeatures);
    instanceInfo.pNext = &validationFeatures;
#endif

    auto vkRes = vk::createInstance(&instanceInfo, m_VulkanAllocationCallbacks, &m_VulkanInstance);
    VK_CHECK(vkRes);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_VulkanInstance);

    const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : instanceExtensions)
    {
        if (std::string(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.debugUtils = true;
        }
    }
}

void VulkanDevice::InitVulkanDebugUtils()
{
    auto severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                         | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    auto bufferUsage = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                       | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                       | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = vk::DebugUtilsMessengerCreateInfoEXT()
                                                         .setMessageSeverity(severityFlags)
                                                         .setMessageType(bufferUsage)
                                                         .setPfnUserCallback(VulkanDebugCallback);
    auto vkRes = m_VulkanInstance.createDebugUtilsMessengerEXT(&debugInfo, m_VulkanAllocationCallbacks,
                                                               &m_VulkanDebugUtilsMessenger);
    VK_CHECK(vkRes);
}

void VulkanDevice::InitVulkanPhysicalDevice(const VulkanDeviceDesc& deviceDesc)
{
    const auto physicalDevices = m_VulkanInstance.enumeratePhysicalDevices();

    vk::PhysicalDevice discreteGPU = nullptr;
    vk::PhysicalDevice integratedGPU = nullptr;

    for (const auto& physicalDevice : physicalDevices)
    {
        m_VulkanPhysicalDeviceProperties = physicalDevice.getProperties();

        if (m_VulkanPhysicalDeviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            discreteGPU = physicalDevice;
            break;
        }

        if (m_VulkanPhysicalDeviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        {
            integratedGPU = physicalDevice;
        }
    }

    if (discreteGPU)
    {
        m_VulkanPhysicalDevice = discreteGPU;
    }
    else if (integratedGPU)
    {
        m_VulkanPhysicalDevice = integratedGPU;
    }
    else
    {
        LOG_FATAL("Failed to find suitable physical device!");
        abort();
    }

    auto extensions = m_VulkanPhysicalDevice.enumerateDeviceExtensionProperties();
    for (const auto& extension : extensions)
    {
        if (std::string(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.timelineSemaphore = true;
        }
        else if (std::string(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.synchronization2 = true;
        }
        else if (std::string(VK_NV_MESH_SHADER_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.meshShaders = true;
        }
        else if (std::string(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.dynamicRendering = true;
        }
        else if (std::string(VK_NV_RAY_TRACING_EXTENSION_NAME) == extension.extensionName)
        {
            m_SupportedExtensions.rayTracing = true;
        }
    }

    m_TimestampFrequency = m_VulkanPhysicalDeviceProperties.limits.timestampPeriod / (1000 * 1000);
    m_UBOAlignment = m_VulkanPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    m_SSBOAlignment = m_VulkanPhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment;

    // Check bindless descriptor support.
    auto indexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeatures();
    auto deviceFeatures2 = vk::PhysicalDeviceFeatures2().setPNext(&indexingFeatures);

    m_VulkanPhysicalDevice.getFeatures2(&deviceFeatures2);
    m_BindlessSupported = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray;

    // Report supported device extensions.
    LOG_TRACE("Supported device extensions/features:");
    LOG_TRACE("Debug utils messenger: ", m_SupportedExtensions.debugUtils);
    LOG_TRACE("Dynamic rendering: ", m_SupportedExtensions.dynamicRendering);
    LOG_TRACE("Mesh shaders: ", m_SupportedExtensions.meshShaders);
    LOG_TRACE("Timeline semaphore: ", m_SupportedExtensions.timelineSemaphore);
    LOG_TRACE("Synchronization 2: ", m_SupportedExtensions.synchronization2);
    LOG_TRACE("Bindless descriptors: ", m_BindlessSupported);
    LOG_TRACE("Ray tracing: ", m_SupportedExtensions.rayTracing);
}

void VulkanDevice::InitVulkanLogicalDevice()
{
    const auto queueFamilies = m_VulkanPhysicalDevice.getQueueFamilyProperties();

    u32 graphicsFamilyIndex = u32(-1);
    u32 computeFamilyIndex = u32(-1);
    u32 transferFamilyIndex = u32(-1);
    u32 presentFamilyIndex = u32(-1);

    // Queue index if compute amily is shared with graphics family.
    u32 computeQueueIndex = u32(-1);

    for (u32 i = 0; i < queueFamilies.size(); i++)
    {
        const auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueCount == 0)
        {
            continue;
        }

        const auto graphicsComputeTransferFlags
            = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;

        // The main graphics queue should be capable for all tasks(including present). There should
        // only be 1 graphics capable queue per GPU.
        if ((queueFamily.queueFlags & graphicsComputeTransferFlags) == graphicsComputeTransferFlags
            && (graphicsFamilyIndex == u32(-1)))
        {
            graphicsFamilyIndex = i;
            if (queueFamily.queueCount > 1)
            {
                computeFamilyIndex = i;
                computeQueueIndex = 1;
            }
        }
        // If compute queue family  has not been selected or it has but the same family as the
        // graphics family is used, assign a new/different family for compute.
        else if ((queueFamily.queueFlags & vk::QueueFlagBits::eCompute)
                 && ((computeFamilyIndex == u32(-1)) || (computeQueueIndex == 1)))
        {
            computeFamilyIndex = i;
            computeQueueIndex = 0;
        }
        // Only assign transfer queue family if there exists a dedicated transfer-only queue.
        else if ((queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
                 && !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) && (transferFamilyIndex == u32(-1)))
        {
            transferFamilyIndex = i;
        }
    }

    m_GraphicsQueueFamily = graphicsFamilyIndex;
    m_ComputeQueueFamily = computeFamilyIndex;
    m_TransferQueueFamily = transferFamilyIndex;

    constexpr float queuePriorities[] = {1.0f, 1.0f};
    std::array<vk::DeviceQueueCreateInfo, 3> queueInfos = {};
    u32 queueCount = 0;

    auto& graphicsQueueInfo = queueInfos[queueCount++];
    graphicsQueueInfo.setQueueFamilyIndex(m_GraphicsQueueFamily)
        .setQueueCount(m_GraphicsQueueFamily == m_ComputeQueueFamily ? 2 : 1)
        .setQueuePriorities(queuePriorities);

    if ((m_ComputeQueueFamily != u32(-1)) && (m_ComputeQueueFamily != m_GraphicsQueueFamily))
    {
        auto& computeQueueInfo = queueInfos[queueCount++];
        computeQueueInfo.setQueueFamilyIndex(m_ComputeQueueFamily).setQueueCount(1).setQueuePriorities(queuePriorities);
    }
    if (m_TransferQueueFamily != u32(-1))
    {
        auto& transferQueueInfo = queueInfos[queueCount++];
        transferQueueInfo.setQueueFamilyIndex(m_TransferQueueFamily)
            .setQueueCount(1)
            .setQueuePriorities(queuePriorities);
    }

    /*
     * Features/extensions chains.
     * XXX: Only enable supported extensions.
     */
    constexpr std::array deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        // Mesh shader pipeline.
        VK_NV_MESH_SHADER_EXTENSION_NAME,

        // Ray tracing pipeline.
        // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    };

    void* currentPNext = nullptr;

    auto vulkan11Features = vk::PhysicalDeviceVulkan11Features().setShaderDrawParameters(true).setPNext(currentPNext);
    currentPNext = &vulkan11Features;

    auto vulkan12Features = vk::PhysicalDeviceVulkan12Features()
                                .setDescriptorIndexing(true)
                                .setRuntimeDescriptorArray(true)
                                .setDescriptorBindingPartiallyBound(true)
                                .setDescriptorBindingVariableDescriptorCount(true)
                                .setTimelineSemaphore(true)
                                .setShaderSampledImageArrayNonUniformIndexing(true)
                                .setPNext(currentPNext);
    currentPNext = &vulkan12Features;

    auto dynamicRenderingFeatures
        = vk::PhysicalDeviceDynamicRenderingFeatures().setDynamicRendering(true).setPNext(currentPNext);
    currentPNext = &dynamicRenderingFeatures;

    auto synchronization2Features
        = vk::PhysicalDeviceSynchronization2FeaturesKHR().setSynchronization2(true).setPNext(currentPNext);
    currentPNext = &synchronization2Features;

    auto meshShaderFeatures
        = vk::PhysicalDeviceMeshShaderFeaturesNV().setMeshShader(true).setTaskShader(true).setPNext(currentPNext);
    currentPNext = &meshShaderFeatures;

    // XXX: Properly chain ray tracing structs.

    // PhysicalDeviceFeatures2 reports the GPU's deviceFeatures capabilites. Pass this along the
    // chain to enable all.
    auto physicalDeviceFeatures2 = m_VulkanPhysicalDevice.getFeatures2();
    physicalDeviceFeatures2.setPNext(currentPNext);
    currentPNext = &physicalDeviceFeatures2;

    auto deviceCreateInfo = vk::DeviceCreateInfo()
                                .setQueueCreateInfoCount(queueCount)
                                .setPQueueCreateInfos(queueInfos.data())
                                .setEnabledExtensionCount(static_cast<u32>(deviceExtensions.size()))
                                .setPpEnabledExtensionNames(deviceExtensions.data())
                                .setPNext(currentPNext);

    vk::Device vulkanDevice;
    const auto vkRes
        = m_VulkanPhysicalDevice.createDevice(&deviceCreateInfo, m_VulkanAllocationCallbacks, &vulkanDevice);
    VK_CHECK(vkRes);

    vulkanDevice.getQueue(graphicsFamilyIndex, 0, &m_GraphicsQueue);
    vulkanDevice.getQueue(computeFamilyIndex, computeQueueIndex, &m_ComputeQueue);
    vulkanDevice.getQueue(transferFamilyIndex, 0, &m_TransferQueue);

    // Use graphics queue for presenting as well.
    m_PresentQueue = m_GraphicsQueue;

    m_VulkanDevice = vulkanDevice;

    LOG_TRACE("Graphics queue family: ", m_GraphicsQueueFamily);
    LOG_TRACE("Compute queue family: ", m_ComputeQueueFamily);
    LOG_TRACE("Transfer queue family: ", m_TransferQueueFamily);
}

void VulkanDevice::InitVulkanAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = m_VulkanPhysicalDevice;
    allocatorInfo.device = m_VulkanDevice;
    allocatorInfo.instance = m_VulkanInstance;

    auto res = vmaCreateAllocator(&allocatorInfo, &m_VmaAllocator);
    assert(res == VK_SUCCESS);
}

void VulkanDevice::InitVulkanGlobalDescriptorPools()
{
    vk::Result vkRes;

    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eSampler, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eCombinedImageSampler, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eSampledImage, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBuffer, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformBuffer, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformBufferDynamic, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eUniformTexelBuffer, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBuffer, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageBufferDynamic, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eStorageTexelBuffer, GPUConstants::GlobalDescriptorPoolElementSize},
        {vk::DescriptorType::eInputAttachment, GPUConstants::GlobalDescriptorPoolElementSize},
    };

    auto poolInfo
        = vk::DescriptorPoolCreateInfo().setMaxSets(GPUConstants::PoolSize::DescriptorSets).setPoolSizes(poolSizes);
    vkRes = m_VulkanDevice.createDescriptorPool(&poolInfo, m_VulkanAllocationCallbacks, &m_VulkanGlobalDescriptorPool);
    VK_CHECK(vkRes);

    if (m_BindlessSupported)
    {
        std::vector<vk::DescriptorPoolSize> poolSizesBindless = {
            {vk::DescriptorType::eSampler, GPUConstants::MaxBindlessResources},
            {vk::DescriptorType::eCombinedImageSampler, GPUConstants::MaxBindlessResources},
        };

        poolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
            .setMaxSets(GPUConstants::MaxBindlessResources * poolSizesBindless.size())
            .setPoolSizes(poolSizesBindless);

        vkRes = m_VulkanDevice.createDescriptorPool(&poolInfo, m_VulkanAllocationCallbacks,
                                                    &m_VulkanGlobalDescriptorPoolBindless);
        VK_CHECK(vkRes);
    }
}

void VulkanDevice::InitThreadFramePools(u32 numThreads, u32 timeQueriesPerFrame)
{
    const u32 numPools = numThreads * GPUConstants::MaxFrames;
    m_ThreadFramePools.resize(numPools);

    // m_GPUTimerQueriesManager.Init(m_ThreadFramePools, timeQueriesPerFrame, numThreads,
    // K_MaxFrames);

    vk::Result vkRes;

    for (u32 i = 0; i < m_ThreadFramePools.size(); i++)
    {
        auto& pool = m_ThreadFramePools[i];
        // pool.timeQueries = &m_GPUTimerQueriesManager.GetQueryTree(i);

        // Create vulkan command buffer pool.
        auto commandPoolInfo = vk::CommandPoolCreateInfo().setQueueFamilyIndex(m_GraphicsQueueFamily);
        vkRes = m_VulkanDevice.createCommandPool(&commandPoolInfo, m_VulkanAllocationCallbacks, &pool.commandPool);
        VK_ASSERT_OK(vkRes);

        // VK best practices validation: Reset command pool instead of passing in reset buffer bit.
        m_VulkanDevice.resetCommandPool(pool.commandPool);

        // Create vulkan timestamp query pool.
        auto timestampPoolInfo
            = vk::QueryPoolCreateInfo().setQueryType(vk::QueryType::eTimestamp).setQueryCount(timeQueriesPerFrame * 2);
        vkRes
            = m_VulkanDevice.createQueryPool(&timestampPoolInfo, m_VulkanAllocationCallbacks, &pool.timestampQueryPool);
        VK_CHECK(vkRes);

        // Create vulkan pipeline statistics query pool.
        vk::QueryPipelineStatisticFlags pipelineStatisticsFlags
            = vk::QueryPipelineStatisticFlagBits::eInputAssemblyVertices
              | vk::QueryPipelineStatisticFlagBits::eInputAssemblyPrimitives
              | vk::QueryPipelineStatisticFlagBits::eVertexShaderInvocations
              | vk::QueryPipelineStatisticFlagBits::eClippingInvocations
              | vk::QueryPipelineStatisticFlagBits::eClippingPrimitives
              | vk::QueryPipelineStatisticFlagBits::eFragmentShaderInvocations
              | vk::QueryPipelineStatisticFlagBits::eComputeShaderInvocations;

        auto pipelineStatisticsInfo = vk::QueryPoolCreateInfo()
                                          .setQueryType(vk::QueryType::ePipelineStatistics)
                                          .setQueryCount(7)
                                          .setPipelineStatistics(pipelineStatisticsFlags);
        vkRes = m_VulkanDevice.createQueryPool(&pipelineStatisticsInfo, m_VulkanAllocationCallbacks,
                                               &pool.pipelineStatsQuerypool);
        VK_CHECK(vkRes);
    }
}

void VulkanDevice::InitVulkanSynchronizationStructures()
{
    vk::Result vkRes;
    auto semaphoreInfo = vk::SemaphoreCreateInfo();

    vkRes
        = m_VulkanDevice.createSemaphore(&semaphoreInfo, m_VulkanAllocationCallbacks, &m_VulkanSemaphoreImageAcquired);
    VK_CHECK(vkRes);

    for (u32 i = 0; i < GPUConstants::MaxFrames; i++)
    {
        vkRes = m_VulkanDevice.createSemaphore(&semaphoreInfo, m_VulkanAllocationCallbacks,
                                               &m_VulkanSemaphoresRenderComplete[i]);
        VK_CHECK(vkRes);
    }

    auto semaphoreTypeInfo = vk::SemaphoreTypeCreateInfo().setSemaphoreType(vk::SemaphoreType::eTimeline);
    semaphoreInfo.setPNext(&semaphoreTypeInfo);

    vkRes = m_VulkanDevice.createSemaphore(&semaphoreInfo, m_VulkanAllocationCallbacks, &m_VulkanSemaphoreGraphics);
    vkRes = m_VulkanDevice.createSemaphore(&semaphoreInfo, m_VulkanAllocationCallbacks, &m_VulkanSemaphoreCompute);
}

std::string_view VulkanDevice::GetGPUName() const
{
    return m_VulkanPhysicalDeviceProperties.deviceName;
}

void VulkanDevice::AdvanceFrameCounters()
{
    m_PreviousFrame = m_CurrentFrame;
    m_CurrentFrame = (m_CurrentFrame + 1) % GPUConstants::MaxFrames;
    m_AbsoluteFrame++;
}

void VulkanDevice::QueueCommandBuffer(CommandBuffer* commandBuffer)
{
    m_QueuedCommandBuffers.push_back(commandBuffer);
}

void VulkanDevice::SubmitQueuedCommandBuffers()
{
    if (m_QueuedCommandBuffers.empty())
    {
        LOG_WARN("No queued command buffers to submit!");
    }

    std::vector<vk::CommandBuffer> enqueuedCommandBuffers;

    for (auto& commandBuffer : m_QueuedCommandBuffers)
    {
        auto vulkanCommandBuffer = commandBuffer->GetVulkanCommandBuffer();
        auto threadFramePools = commandBuffer->GetThreadFramePools();

        enqueuedCommandBuffers.push_back(vulkanCommandBuffer);

        // commandBuffer->EndCurrentRenderPass();

        if (threadFramePools->timeQueries->allocatedTimeQuery)
        {
            vulkanCommandBuffer.endQuery(threadFramePools->pipelineStatsQuerypool, 0);
        }

        vulkanCommandBuffer.end();

        // commandBuffer->SetIsRecording(false);
        commandBuffer->SetCurrentRenderPass(nullptr);
    }

    u32 waitSemaphoreCount = 1;

    bool waitGraphicsTimelineSemaphore = (m_AbsoluteFrame >= GPUConstants::MaxFrames);
    if (waitGraphicsTimelineSemaphore)
    {
        waitSemaphoreCount++;
    }

    bool waitComputeSemaphore = (m_LastComputeSemaphoreValue > 0) && m_HasAsyncWork;
    if (waitComputeSemaphore)
    {
        waitSemaphoreCount++;
    }

    std::vector<vk::CommandBufferSubmitInfo> commandBufferSubmitInfos;
    for (const auto commandBuffer : enqueuedCommandBuffers)
    {
        commandBufferSubmitInfos.push_back(vk::CommandBufferSubmitInfo().setCommandBuffer(commandBuffer));
    }

    auto renderCompleteSemaphore = m_VulkanSemaphoresRenderComplete[m_CurrentFrame];

    std::vector<vk::SemaphoreSubmitInfo> waitSemaphoresInfo = {
        // Image acquired semaphore.
        vk::SemaphoreSubmitInfo()
            .setSemaphore(m_VulkanSemaphoreImageAcquired)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
        // Graphics work timeline semaphore.
        // XXX: Do we need to wait for this? Wait for this semaphore is already called in NewFrame().
        vk::SemaphoreSubmitInfo()
            .setSemaphore(m_VulkanSemaphoreGraphics)
            .setValue(GetGraphicsSemaphoreWaitValue())
            .setStageMask(vk::PipelineStageFlagBits2::eTopOfPipe),
        // Async compute semaphore.
        // XXX: Only add this IFF there is compute work.
        vk::SemaphoreSubmitInfo()
            .setSemaphore(m_VulkanSemaphoreCompute)
            .setValue(m_LastComputeSemaphoreValue)
            .setStageMask(vk::PipelineStageFlagBits2::eVertexAttributeInput),
    };

    std::vector<vk::SemaphoreSubmitInfo> signalSemaphoresInfo = {
        // Present/render complete semaphore.
        vk::SemaphoreSubmitInfo()
            .setSemaphore(renderCompleteSemaphore)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
        // New graphics work timeline semaphore value.
        vk::SemaphoreSubmitInfo()
            .setSemaphore(m_VulkanSemaphoreGraphics)
            .setValue(m_AbsoluteFrame + 1)
            .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput),
    };

    auto submitInfo = vk::SubmitInfo2()
                          .setWaitSemaphoreInfoCount(waitSemaphoreCount)
                          .setPWaitSemaphoreInfos(waitSemaphoresInfo.data())
                          .setCommandBufferInfos(commandBufferSubmitInfos)
                          .setSignalSemaphoreInfos(signalSemaphoresInfo);
    m_GraphicsQueue.submit2(submitInfo);

    m_QueuedCommandBuffers.clear();
}

void VulkanDevice::SubmitComputeCommandBuffer(CommandBuffer* commandBuffer)
{
    assert(commandBuffer);

    m_HasAsyncWork = true;

    const bool hasWaitSemaphore = (m_LastComputeSemaphoreValue) > 0;

    auto waitInfo = vk::SemaphoreSubmitInfo()
                        .setSemaphore(m_VulkanSemaphoreCompute)
                        .setValue(m_LastComputeSemaphoreValue)
                        .setStageMask(vk::PipelineStageFlagBits2::eComputeShader);

    m_LastComputeSemaphoreValue++;

    auto signalInfo = vk::SemaphoreSubmitInfo()
                          .setSemaphore(m_VulkanSemaphoreCompute)
                          .setValue(m_LastComputeSemaphoreValue)
                          .setStageMask(vk::PipelineStageFlagBits2::eComputeShader);

    auto commandBufferInfo = vk::CommandBufferSubmitInfo().setCommandBuffer(commandBuffer->GetVulkanCommandBuffer());

    auto submitInfo = vk::SubmitInfo2()
                          .setWaitSemaphoreInfoCount(hasWaitSemaphore ? 1 : 0)
                          .setPWaitSemaphoreInfos(&waitInfo)
                          .setCommandBufferInfos(commandBufferInfo)
                          .setSignalSemaphoreInfos(signalInfo);

    m_ComputeQueue.submit2(submitInfo);
}

void VulkanDevice::Present(CommandBuffer* asyncComputeCommandBuffer)
{
    vk::Result vkRes;

    vkRes = m_Swapchain.AcquireNextImage(m_VulkanSemaphoreImageAcquired);
    if (vkRes == vk::Result::eErrorOutOfDateKHR)
    {
        m_Swapchain.Resize();
        AdvanceFrameCounters();
        return;
    }

    SubmitQueuedCommandBuffers();

    m_HasAsyncWork = false;
    if (asyncComputeCommandBuffer != nullptr)
    {
        SubmitComputeCommandBuffer(asyncComputeCommandBuffer);
    }

    const auto renderCompleteSemaphore = m_VulkanSemaphoresRenderComplete[m_CurrentFrame];

    vkRes = m_Swapchain.Present(renderCompleteSemaphore);

    if (m_TimestampsEnabled)
    {
        // XXX: Implement get query results.
        // GetQueryPoolResults();
    }

    if ((vkRes == vk::Result::eErrorOutOfDateKHR) || (vkRes == vk::Result::eSuboptimalKHR) || m_Resized)
    {
        m_Resized = false;
        m_Swapchain.Resize();
        AdvanceFrameCounters();
        return;
    }

    AdvanceFrameCounters();

    // Update bindless textures.
    if (!m_BindlessTextureUpdates.empty())
    {
        // XXX: Implement bindless resource update.
        // UpdateBindlessTextures();
    }

    // XXX: Destroy queued GPU resource here? Have resource destructors queue objects intstead of
    // destroying them directly?
    m_DeferredDeletionQueue.DestroyResources();
}

void VulkanDevice::NewFrame()
{
    if (m_AbsoluteFrame >= GPUConstants::MaxFrames)
    {
        u64 graphicsTimelineValue = GetGraphicsSemaphoreWaitValue();
        u64 computeTimelineValue = m_LastComputeSemaphoreValue;

        u64 waitValues[] = {graphicsTimelineValue, computeTimelineValue};
        vk::Semaphore semaphores[] = {m_VulkanSemaphoreGraphics, m_VulkanSemaphoreCompute};

        auto semaphoreWaitInfo = vk::SemaphoreWaitInfo()
                                     .setSemaphoreCount(m_HasAsyncWork ? 2 : 1)
                                     .setPSemaphores(semaphores)
                                     .setPValues(waitValues);

        const auto vkRes = m_VulkanDevice.waitSemaphores(semaphoreWaitInfo, u64(-1));
        VK_CHECK(vkRes);
    }

    // XXX: Reset command pools.
    // m_CommandBufferManager.ResetPools();

    // XXX: Update descriptor sets.
    if (!m_DescriptorSetUpdates.empty())
    {
        for (const auto& update : m_DescriptorSetUpdates)
        {
            // UpdateDescriptorSet(update.descriptorSet);
        }

        m_DescriptorSetUpdates.clear();
    }

    // XXX: Reset time queries
    for (u32 i = 0; i < m_ThreadFramePools.size() / GPUConstants::MaxFrames; ++i)
    {
        // GpuThreadFramePools& thread_pool = thread_frame_pools[(current_frame * num_threads) + i];
        // thread_pool.time_queries->reset();
    }
}

void VulkanDevice::Resize()
{
    m_Resized = true;
}
void VulkanDevice::QueueUpdateDescriptorSet(DescriptorSet* set)
{
    assert(set);
    m_DescriptorSetUpdates.push_back(DescriptorSetUpdate{
        .descriptorSet = set,
    });
}

void VulkanDevice::LinkTextureSampler(Texture* texture, Sampler* sampler) const
{
    texture->m_Sampler = sampler;
}

void VulkanDevice::SetVulkanHandleResourceName(vk::ObjectType objectType, u64 handle, const std::string& name)
{
    auto nameInfo = vk::DebugUtilsObjectNameInfoEXT()
                        .setObjectType(objectType)
                        .setObjectHandle(handle)
                        .setPObjectName(name.c_str());

    auto res = m_VulkanDevice.setDebugUtilsObjectNameEXT(&nameInfo);
    VK_CHECK(res);
}

void* VulkanDevice::MapBuffer(const MapBufferParameters& params)
{
    auto buffer = params.buffer;
    if (buffer == nullptr)
    {
        return nullptr;
    }

    // if (buffer->m_ParentBuffer == m_DynamicBuffer)
    // {
    // buffer->m_GlobalOffset = dynamic_allocated_size;
    // return dynamic_allocate(parameters.size == 0 ? buffer->size : parameters.size);
    // }

    void* data;
    vmaMapMemory(m_VmaAllocator, buffer->m_VmaAllocation, &data);

    return data;
}

void VulkanDevice::UnmapBuffer(const MapBufferParameters& params)
{
    auto buffer = params.buffer;
    if (buffer == nullptr)
    {
        return;
    }

    // if (buffer->m_ParentBuffer == m_DynamicBuffer)
    // {
    // }

    vmaUnmapMemory(m_VmaAllocator, buffer->m_VmaAllocation);
}

// XXX: Implement this.
// void* VulkanDevice::DynamicAllocate(u32 size)
// {
//     void* mappedMemory = m_DynamicMappedMemory + m_DynamicAllocatedSize;
//     // dynamic_allocated_size += (u32)raptor::memory_align(size, ubo_alignment);
//     return mappedMemory;
// }

CommandBuffer* VulkanDevice::GetCommandBuffer(u32 frameIndex, u32 threadIndex, bool begin)
{
    return m_CommandBufferManager.GetCommandBuffer(frameIndex, threadIndex, begin);
}

CommandBuffer* VulkanDevice::GetCommandBufferSecondary(u32 frameIndex, u32 threadIndex)
{
    return m_CommandBufferManager.GetCommandBufferSecondary(frameIndex, threadIndex);
}

void VulkanDevice::AddBindlessResourceUpdate(const ResourceUpdate& update)
{
    m_BindlessTextureUpdates.push_back(update);
}
