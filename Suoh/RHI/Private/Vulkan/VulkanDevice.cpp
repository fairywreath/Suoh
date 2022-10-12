#include "VulkanDevice.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanDevice::VulkanDevice(const DeviceDesc& desc) : m_DeviceManager(m_Instance)
{
    auto deviceContext = m_DeviceManager.CreateDevice(desc);

    m_Device = deviceContext.device;
    m_PhysDevice = deviceContext.physicalDevice;
}

VulkanDevice::~VulkanDevice()
{
    m_Device.destroy();
}

GraphicsAPI VulkanDevice::GetGraphicsAPI() const
{
    return GraphicsAPI::VULKAN;
}

HeapHandle VulkanDevice::CreateHeap(const HeapDesc& desc)
{
    return HeapHandle(nullptr);
}

BufferHandle VulkanDevice::CreateBuffer(const BufferDesc& desc)
{
    return BufferHandle();
}

void* VulkanDevice::MapBuffer(IBuffer* buffer, CpuAccessMode access)
{
    return nullptr;
}

void VulkanDevice::UnmapBuffer(IBuffer* buffer)
{
}

MemoryRequirements VulkanDevice::GetBufferMemoryRequirements(IBuffer* buffer)
{
    return MemoryRequirements();
}

bool VulkanDevice::BindBufferMemory(IBuffer* buffer, IHeap* heap, u64 offset)
{
    return false;
}

BufferHandle VulkanDevice::CreateHandleForNativeBuffer(ObjectType objectType, Object buffer, const BufferDesc& desc)
{
    return BufferHandle();
}

TextureHandle VulkanDevice::CreateTexture(const TextureDesc& desc)
{
    return TextureHandle();
}

MemoryRequirements VulkanDevice::GetTextureMemoryRequireents(ITexture* texture)
{
    return MemoryRequirements();
}

bool VulkanDevice::BindTextureMemory(ITexture* texture, IHeap* heap, u64 offset)
{
    return false;
}

TextureHandle VulkanDevice::CreateHandleForNativeTexture(ObjectType objectType, Object texture, const TextureDesc& desc)
{
    return TextureHandle();
}

StagingTextureHandle VulkanDevice::CreateStagingTexture(const TextureDesc& desc, CpuAccessMode access)
{
    return StagingTextureHandle();
}

void* VulkanDevice::MapStagingTexture(IStagingTexture* texture, const TextureLayer& textureLayer, CpuAccessMode access, size_t* outRowPitch)
{
    return nullptr;
}

void VulkanDevice::UnmapStagingTexture(IStagingTexture* texture)
{
}

ShaderHandle VulkanDevice::CreateShader(const ShaderDesc& desc, const void* binary, size_t binarySize)
{
    return ShaderHandle();
}

ShaderLibraryHandle VulkanDevice::CreateShaderLibrary(const void* binary, size_t binarySize)
{
    return ShaderLibraryHandle();
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDesc& desc)
{
    return SamplerHandle();
}

InputLayoutHandle VulkanDevice::CreateInputLayout(const VertexAttributeDesc* desc, u32 attributeCount, IShader* vertexShader)
{
    return InputLayoutHandle();
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
    return FramebufferHandle();
}

GraphicsPipelineHandle VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    return GraphicsPipelineHandle();
}

ComputePipelineHandle VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    return ComputePipelineHandle();
}

BindingLayoutHandle VulkanDevice::CreateBindingLayout(const BindingLayoutDesc& desc)
{
    return BindingLayoutHandle();
}

BindingLayoutHandle VulkanDevice::CreateBindlessLayout(const BindlessLayoutDesc& desc)
{
    return BindingLayoutHandle();
}

BindingSetHandle VulkanDevice::CreateBindingSet(const BindingSetDesc& desc, IBindingLayout* layout)
{
    return BindingSetHandle();
}

DescriptorTableHandle VulkanDevice::CreateDescriptorTable(IBindingLayout* layout)
{
    return DescriptorTableHandle();
}

void VulkanDevice::ResizeDescriptorTable(IDescriptorTable* descriptorTable, u32 newSize, bool keepContents)
{
}

bool VulkanDevice::WriteDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& item)
{
    return false;
}

CommandListHandle VulkanDevice::CreateCommandList(const CommandListParameters& params)
{
    return CommandListHandle();
}

u64 VulkanDevice::ExecuteCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists, CommandQueue executionQueue)
{
    return u64();
}

void VulkanDevice::QueueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, u64 instance)
{
}

void VulkanDevice::WaitForIdle()
{
}

void VulkanDevice::Present(ITexture* texture)
{
}

void VulkanDevice::RunGarbageCollection()
{
}

Object VulkanDevice::GetNativeQueue(ObjectType objectType, CommandQueue queue)
{
    if (queue == CommandQueue::GRAPHICS)
        return Object(m_GraphicsQueue);
    else
        return Object(m_PresentQueue);
}

bool VulkanDevice::QueryFeatureSupport(RHIFeature feature, void* pInfo, size_t infoSize)
{
    return false;
}

FormatSupport VulkanDevice::QueryFormatSupport(Format format)
{
    return FormatSupport();
}

vk::Queue VulkanDevice::GetGraphicsQueue() const
{
    return vk::Queue();
}

u32 VulkanDevice::GetGraphicsFamily() const
{
    return u32();
}

vk::Queue VulkanDevice::GetComputeQueue() const
{
    return vk::Queue();
}

u32 VulkanDevice::GetComputeFamily() const
{
    return u32();
}

bool VulkanDevice::ComputeCapable() const
{
    return false;
}

vk::Queue VulkanDevice::GetPresentQueue() const
{
    return vk::Queue();
}

u32 VulkanDevice::GetPresentFamily() const
{
    return u32();
}

} // namespace Vulkan

} // namespace SuohRHI