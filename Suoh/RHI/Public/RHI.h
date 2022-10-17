#pragma once

#include "RHIContainers.h"
#include "RHIRenderStates.h"
#include "RHIResources.h"

#include <string>
#include <variant>

namespace SuohRHI
{

/*
 * Texture.
 */
struct TextureDesc
{
    u32 width{1};
    u32 height{1};
    u32 depth{1};

    // XXX: properly implement this from GetFormatInfo
    bool hasDepth{false};
    bool hasStencil{false};

    u32 mipLevels{1};
    u32 layerCount{0};

    u32 sampleCount{0};
    u32 sampleQuality{0};

    Format format{Format::UNKNOWN};
    TextureDimension dimension{TextureDimension::TEXTURE_2D};

    std::string debugName;

    bool isRenderTarget{false};

    Color clearValue;
    bool useClearValue{false};

    ResourceStates initialState{ResourceStates::UNKNOWN};
    bool keepInitialState{false};

    // UAV
    bool isStorage{false};
};

struct TextureLayer
{
    u32 x{0};
    u32 y{0};
    u32 z{0};

    u32 width{u32(-1)};
    u32 height{u32(-1)};
    u32 depth{u32(-1)};

    MipLevel mipLevel{0};
    ArrayLayer arrayLayer{0};

    [[nodiscard]] TextureLayer Resolve(const TextureDesc& desc) const;
};

struct TextureSubresourceSet
{
    TextureSubresourceSet() = default;
    TextureSubresourceSet(MipLevel baseMipLevel, MipLevel numMipLevels, ArrayLayer baseArrayLayer, ArrayLayer numArrayLayers)
        : baseMipLevel(baseMipLevel), numMipLevels(numMipLevels), baseArrayLayer(baseArrayLayer), numArrayLayers(numArrayLayers){};

    bool operator==(const TextureSubresourceSet& other) const
    {
        return baseMipLevel == other.baseMipLevel && numMipLevels == other.numMipLevels && baseArrayLayer == other.baseArrayLayer
               && numArrayLayers == other.numArrayLayers;
    }

    [[nodiscard]] TextureSubresourceSet Resolve(const TextureDesc& desc, bool singleMipLevel) const;
    [[nodiscard]] bool IsEntireTexture(const TextureDesc& desc) const;

    static constexpr MipLevel AllMipLevels{1};
    static constexpr ArrayLayer AllArrayLayers{1};

    MipLevel baseMipLevel{0};
    MipLevel numMipLevels{1};
    ArrayLayer baseArrayLayer{0};
    ArrayLayer numArrayLayers{1};
};

static const TextureSubresourceSet AllSubresources
    = TextureSubresourceSet(0, TextureSubresourceSet::AllMipLevels, 0, TextureSubresourceSet::AllArrayLayers);

class ITexture : public IResource
{
public:
    [[nodiscard]] virtual const TextureDesc& GetDesc() const = 0;

    virtual Object GetNativeView(ObjectType objectType, Format format = Format::UNKNOWN,
                                 TextureSubresourceSet subresources = AllSubresources,
                                 TextureDimension dimension = TextureDimension::UNKNOWN)
        = 0;
};

using TextureHandle = RefCountPtr<ITexture>;

class IStagingTexture : public IResource
{
public:
    [[nodiscard]] virtual const TextureDesc& GetDesc() const = 0;
};

using StagingTextureHandle = RefCountPtr<IStagingTexture>;

/*
 * Input Layout.
 */
struct VertexAttributeDesc
{
    std::string name;
    Format format{Format::UNKNOWN};
    u32 arraySize{1};
    u32 bufferIndex{0};
    u32 offSet{0};
    u32 elementStride{0};
    bool isInstanced{false};
};

class IInputLayout : public IResource
{
public:
    [[nodiscard]] virtual u32 GetNumAttributes() const = 0;
    [[nodiscard]] virtual const VertexAttributeDesc* GetAttributeDesc(u32 index) const = 0;
};

using InputLayoutHandle = RefCountPtr<IInputLayout>;

/*
 * Buffer.
 */
struct BufferDesc
{
    u64 byteSize;
    BufferUsage usage{BufferUsage::UNKNOWN};

    std::string debugName;
    Format format{Format::UNKNOWN};

    bool isVolatile{false};
    bool keepInitialState{false};

    CpuAccessMode cpuAccess{CpuAccessMode::NONE};

    // Can have UAVs.
    bool supportStorageBuffer{false};

    ResourceStates initialState = ResourceStates::COMMON;
};

struct BufferRange
{
    BufferRange() = default;
    BufferRange(u64 offset, u64 size) : byteOffset(offset), byteSize(size){};

    u64 byteOffset{0};
    u64 byteSize{0};

    [[nodiscard]] BufferRange Resolve(const BufferDesc& desc) const;
    [[nodiscard]] constexpr bool IsEntireBuffer(const BufferDesc& desc) const
    {
        return ((byteOffset == 0) && (byteSize == ~0ull)) || (byteSize == desc.byteSize);
    }

    constexpr bool operator==(const BufferRange& other)
    {
        return (byteOffset == other.byteOffset) && (byteSize == other.byteSize);
    }
};

static const BufferRange EntireBuffer = BufferRange(0, ~0ull);

class IBuffer : public IResource
{
public:
    [[nodiscard]] virtual const BufferDesc& GetDesc() const = 0;
};

using BufferHandle = RefCountPtr<IBuffer>;

/*
 * Shader
 * XXX: Need to implemenent shader specializations.
 */
struct ShaderDesc
{
    ShaderDesc() = default;
    ShaderDesc(ShaderType type) : shaderType(type){};

    ShaderType shaderType{ShaderType::NONE};

    std::string entryName{"main"};
    std::string debugName;
};

class IShader : public IResource
{
public:
    [[nodiscard]] virtual const ShaderDesc& GetDesc() const = 0;
    virtual void GetBytecode(const void** ppByteCode, size_t* pSize) const = 0;
};

using ShaderHandle = RefCountPtr<IShader>;

/*
 * Shader Library.
 */
class IShaderLibrary : public IResource
{
public:
    virtual ShaderHandle GetShader(const char* entryName, ShaderType shaderType) = 0;
    virtual void GetBytecode(const void** ppByteCode, size_t* pSize) const = 0;
};

using ShaderLibraryHandle = RefCountPtr<IShaderLibrary>;

/*
 * Sampler.
 */
using SamplerDesc = Sampler;

class ISampler : public IResource
{
public:
    [[nodiscard]] virtual const SamplerDesc& GetDesc() const = 0;
};

using SamplerHandle = RefCountPtr<ISampler>;

/*
 * Framebuffer.
 */
struct FramebufferAttachment
{
    ITexture* texture{nullptr};
    TextureSubresourceSet subresources{TextureSubresourceSet(0, 1, 0, 1)};
    Format format{Format::UNKNOWN};
    bool isReadOnly{false};

    [[nodiscard]] bool Valid() const
    {
        return (texture != nullptr);
    }
};

struct FramebufferDesc
{
    static_vector<FramebufferAttachment, MAX_RENDER_TARGETS> colorAttachments;
    FramebufferAttachment depthAttachment;

    // XXX: Implement shading rate attachment
};

struct FramebufferInfo
{
    FramebufferInfo() = default;
    FramebufferInfo(const FramebufferDesc& desc);

    static_vector<Format, MAX_RENDER_TARGETS> colorFormats;
    Format depthFormat{Format::UNKNOWN};
    u32 width{0};
    u32 height{0};
    u32 sampleCount{1};
    u32 sampleQuality{0};
};

class IFramebuffer : public IResource
{
public:
    [[nodiscard]] virtual const FramebufferDesc& GetDesc() const = 0;
    [[nodiscard]] virtual const FramebufferInfo& GetFramebufferInfo() const = 0;
};

using FramebufferHandle = RefCountPtr<IFramebuffer>;

/*
 * Bindings.
 */
struct BindingLayoutItem
{
    u32 slot;

    BindingResourceType type : 8;
    u8 unused : 8;
    u16 size : 16;
};

static constexpr auto MAX_BINDINGS_PER_LAYOUT = 128;
static constexpr auto C_MAX_BINDING_LAYOUTS = 4;
static constexpr auto C_MAX_VOLATILE_CONSTANT_BUFFERS = 32;

using BindingLayoutItemArray = static_vector<BindingLayoutItem, MAX_BINDINGS_PER_LAYOUT>;

template <typename T> using BindingVector = static_vector<T, C_MAX_BINDING_LAYOUTS>;

struct BindingLayoutDesc
{
    // ShaderVisibility visibility{ShaderVisibility::NONE};
    ShaderType visibility{ShaderType::NONE};

    u32 registerSpace{0};
    BindingLayoutItemArray bindings;

    // XXX: Need custom vulkan binding offsets for HLSL -> SPIRV compilation.
    // VulkanBindingOffsets....
};

/*
 * Bindless: binding slot/point is set automatically inside RHI.
 */
struct BindlessLayoutDesc
{
    ShaderType visibility{ShaderType::NONE};

    // slot starts from 0
    // u32 firstSlot{0};

    u32 maxCapacity{0};
    static_vector<BindingLayoutItem, 16> registerSpaces;
};

class IBindingLayout : public IResource
{
public:
    [[nodiscard]] virtual const BindingLayoutDesc* GetDesc() const = 0;
    [[nodiscard]] virtual const BindlessLayoutDesc* GetBindlessDesc() const = 0;
};

using BindingLayoutHandle = RefCountPtr<IBindingLayout>;
using BindingLayoutVector = static_vector<BindingLayoutHandle, MAX_BINDINGS_PER_LAYOUT>;

/*
 * Descriptor/binding Sets.
 * XXX: have a larger DescriptorSet class for a more "Vulkan" design approach?
 */
struct BindingSetItem
{
    IResource* resourceHandle;

    u32 slot;

    BindingResourceType type : 8;
    TextureDimension dimension : 8;
    Format format : 32;
    u8 unused : 8;

    // std::variant<TextureSubresourceSet, BufferRange, u64[2]> rawData;
    union {
        TextureSubresourceSet textureSubresources;
        BufferRange bufferRange;
        u64 rawData[2];
    };

    BindingSetItem()
    {
    }
    // XXX: Constructors for each binding item type.
};

using BindingSetItemArray = static_vector<BindingSetItem, MAX_BINDINGS_PER_LAYOUT>;

struct BindingSetDesc
{
    BindingSetItemArray bindings;
};

class IBindingSet : public IResource
{
public:
    [[nodiscard]] virtual const BindingSetDesc* GetDesc() const = 0;
    [[nodiscard]] virtual IBindingLayout* GetLayout() const = 0;
};

using BindingSetHandle = RefCountPtr<IBindingSet>;
using BindingSetVector = static_vector<IBindingSet*, MAX_BINDINGS_PER_LAYOUT>;

/*
 * For "bindless" layour desc.
 */
class IDescriptorTable : public IBindingSet
{
public:
    [[nodiscard]] virtual u32 GetCapacity() const = 0;
};

using DescriptorTableHandle = RefCountPtr<IDescriptorTable>;

/*
 * Graphics Pipeline.
 */
struct GraphicsPipelineDesc
{
    PrimitiveTopology primitiveTopology{PrimitiveTopology::TRIANGLE_STRIP};
    u32 patchControlPoints{0};
    InputLayoutHandle inputLayout;

    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    ShaderHandle geometryShader;

    RenderState renderState;

    BindingLayoutVector bindingLayouts;
};

class IGraphicsPipeline : public IResource
{
public:
    [[nodiscard]] virtual const GraphicsPipelineDesc& GetDesc() const = 0;
    [[nodiscard]] virtual const FramebufferInfo& GetFramebufferInfo() const = 0;
};

using GraphicsPipelineHandle = RefCountPtr<IGraphicsPipeline>;

/*
 * Compute pipeline
 */
struct ComputePipelineDesc
{
    ShaderHandle computeShader;
    BindingLayoutVector bindingLayouts;
};

class IComputePipeline : public IResource
{
public:
    [[nodiscard]] virtual const ComputePipelineDesc& GetDesc() const = 0;
};

using ComputePipelineHandle = RefCountPtr<IComputePipeline>;

/*
 * Swapchain.
 */
class Swapchain
{
public:
    virtual ~Swapchain();
};

/*
 * Dispatch
 */

// util queries
class IEventQuery : public IResource
{
};
using EventQueryHandle = RefCountPtr<IEventQuery>;

class ITimerQuery : public IResource
{
};
using TimerQueryHandle = RefCountPtr<ITimerQuery>;

struct VertexBufferBinding
{
    IBuffer* buffer{nullptr};
    u32 slot;
    u32 offset;

    bool operator==(const VertexBufferBinding& b) const
    {
        return buffer == b.buffer && slot == b.slot && offset == b.offset;
    }
};

struct IndexBufferBinding
{
    IBuffer* buffer{nullptr};
    Format format;
    u32 offset;

    bool operator==(const IndexBufferBinding& b) const
    {
        return buffer == b.buffer && format == b.format && offset == b.offset;
    }
};

static constexpr auto MAX_VERTEX_ATTRIBUTES = 16;
struct GraphicsState
{
    IGraphicsPipeline* pipeline{nullptr};
    IFramebuffer* framebuffer{nullptr};

    Color blendConstantColor;
    ViewportState viewportState;

    BindingSetVector bindings;

    static_vector<VertexBufferBinding, MAX_VERTEX_ATTRIBUTES> vertexBuffers;
    IndexBufferBinding indexBuffer;

    IBuffer* indirectParams{nullptr};
};

struct DrawArguments
{
    u32 vertexCount{0};
    u32 instanceCount{1};
    u32 startIndexLocation{0};
    u32 startVertexLocation{0};
    u32 startInstanceLocation{0};
};

struct ComputeState
{
    IComputePipeline* pipeline{nullptr};
    BindingSetVector bindings;
    IBuffer* indirectParams{nullptr};
};

/*
 * Misc. RHI features
 */
enum class RHIFeature : u8
{
    DEFERRED_COMMANDLISTS,
    COMPUTE_QUEUE,
};

enum class CommandQueue : u8
{
    GRAPHICS = 0,
    PRESENT = 1,
    COMPUTE = 2,
    COUNT,
};

class IDevice;

struct CommandListParameters
{
    CommandQueue queueType{CommandQueue::GRAPHICS};
};

/*
 * Command List.
 */
class ICommandList : public IResource
{
public:
    virtual IDevice* GetDevice() = 0;
    virtual const CommandListParameters& GetDesc() const = 0;

    virtual Object GetNativeObject(ObjectType type) = 0;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void ClearState() = 0;
    virtual void ClearTexture(ITexture* texture, TextureSubresourceSet subresources, const Color& clearColor) = 0;
    virtual void ClearTextureUint(ITexture* texture, TextureSubresourceSet subresources, u32 clearColor) = 0;
    virtual void ClearDepthStencilTexture(ITexture* texture, TextureSubresourceSet subresources, bool clearDepth, float depth,
                                          bool clearStencil, u8 stencil)
        = 0;

    virtual void CopyTexture(ITexture* dest, const TextureLayer& destLayer, ITexture* src, const TextureLayer* srcLayer) = 0;
    virtual void CopyTexture(IStagingTexture* dest, const TextureLayer& destLayer, ITexture* src, const TextureLayer& srcLayer) = 0;
    virtual void CopyTexture(ITexture* dest, const TextureLayer& destLayer, IStagingTexture* src, const TextureLayer& srcLayer) = 0;
    virtual void WriteTexture(ITexture* dest, u32 arrayLayer, u32 mipLevel, const void* data, size_t rowPitch, size_t depthPitch = 0) = 0;
    virtual void ResolveTexture(ITexture* dest, const TextureSubresourceSet& dstSubresources, ITexture* src,
                                const TextureSubresourceSet& srcSubresources)
        = 0;

    virtual void WriteBuffer(IBuffer* b, const void* data, size_t dataSize, u64 destOffsetBytes = 0) = 0;
    virtual void ClearBufferUInt(IBuffer* b, u32 clearValue) = 0;
    virtual void CopyBuffer(IBuffer* dest, u64 destOffsetBytes, IBuffer* src, u64 srcOffsetBytes, u64 dataSizeBytes) = 0;

    virtual void SetPushConstants(const void* data, size_t byteSize) = 0;

    virtual void SetGraphicsState(const GraphicsState& state) = 0;

    /*
     * XXX: Should we pass in buffer handles when doing draw calls?
     * Current design the curent buffer is set first from SetBufferState.
     * Passing in buffer handles directly might be a better approach, same goes for all
     * operations for textures.
     */
    virtual void Draw(const DrawArguments& args) = 0;
    virtual void DrawIndexed(const DrawArguments& args) = 0;
    virtual void DrawIndirect(u32 offsetBytes) = 0;
    virtual void DrawIndirectCount(u32 offsetBytes, u32 drawCount) = 0;

    virtual void SetComputeState(const ComputeState& state) = 0;
    virtual void Dispatch(u32 groupsX, u32 groupsY = 1, u32 groupsZ = 1) = 0;
    virtual void DispatchIndirect(u32 offsetBytes) = 0;

    virtual void BeginTimerQuery(ITimerQuery* query) = 0;
    virtual void EndTimerQuery(ITimerQuery* query) = 0;

    virtual void BeginMarker(const char* name) = 0;
    virtual void EndMarker() = 0;

    virtual void SetEnableAutomaticBarriers(bool enable) = 0;
    virtual void SetResourceStatesForBindingSet(IBindingSet* bindingSet) = 0;
    void SetResourceStatesForFramebuffer(IFramebuffer* framebuffer);

    virtual void BeginTrackingTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) = 0;
    virtual void BeginTrackingBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;

    virtual void SetTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) = 0;
    virtual void SetBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;
    virtual void SetPermanentTextureState(ITexture* texture, ResourceStates stateBits) = 0;
    virtual void SetPermanentBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;

    virtual void SetEnableSSBOBarriersForTexture(ITexture* texture, bool enableBarriers) = 0;
    virtual void SetEnableSSBOBarriersForBuffer(IBuffer* buffer, bool enableBarriers) = 0;

    virtual void CommitBarriers() = 0;

    virtual ResourceStates GetTextureSubresourceState(ITexture* texture, ArrayLayer arrayLayer, MipLevel mipLevel) = 0;
    virtual ResourceStates GetBufferState(IBuffer* buffer) = 0;
};

using CommandListHandle = RefCountPtr<ICommandList>;

/*
 * Render Device.
 */
class IDevice : public IResource
{
public:
    virtual GraphicsAPI GetGraphicsAPI() const = 0;

    /*
     * Buffers.
     */
    virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
    virtual BufferHandle CreateHandleForNativeBuffer(ObjectType objectType, Object buffer, const BufferDesc& desc) = 0;
    virtual void* MapBuffer(IBuffer* buffer, CpuAccessMode access) = 0;
    virtual void UnmapBuffer(IBuffer* buffer) = 0;

    /*
     * Texture/images.
     */
    virtual TextureHandle CreateTexture(const TextureDesc& desc) = 0;
    virtual TextureHandle CreateHandleForNativeTexture(ObjectType objectType, Object texture, const TextureDesc& desc) = 0;

    virtual StagingTextureHandle CreateStagingTexture(const TextureDesc& desc, CpuAccessMode access) = 0;
    virtual void* MapStagingTexture(IStagingTexture* texture, const TextureLayer& textureLayer, CpuAccessMode access, size_t* outRowPitch)
        = 0;
    virtual void UnmapStagingTexture(IStagingTexture* texture) = 0;

    /*
     * Shaders.
     */
    virtual ShaderHandle CreateShader(const ShaderDesc& desc, const void* binary, size_t binarySize) = 0;
    virtual ShaderLibraryHandle CreateShaderLibrary(const void* binary, size_t binarySize) = 0;

    /*
     * Samplers.
     */
    virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
    virtual InputLayoutHandle CreateInputLayout(const VertexAttributeDesc* desc, u32 attributeCount, IShader* vertexShader) = 0;

    /*
     * Event/timer Queries.
     */
    virtual EventQueryHandle CreateEventQuery() = 0;
    virtual void SetEventQuery(IEventQuery* query, CommandQueue queue) = 0;
    virtual bool PollEventQuery(IEventQuery* query) = 0;
    virtual void WaitEventQuery(IEventQuery* query) = 0;
    virtual void ResetEventQuery(IEventQuery* query) = 0;

    virtual TimerQueryHandle CreateTimerQuery() = 0;
    virtual bool PollTimerQuery(ITimerQuery* query) = 0;
    virtual float GetTimerQueryTimeSeconds(ITimerQuery* query) = 0;
    virtual void ResetTimerQuerySeconds(ITimerQuery* query) = 0;

    /*
     * Graphics Pipeline.
     */
    virtual FramebufferHandle CreateFramebuffer(const FramebufferDesc& desc) = 0;
    virtual GraphicsPipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc, IFramebuffer* framebuffer) = 0;

    /*
     * Compute.
     */
    virtual ComputePipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;

    /*
     * Descriptor sets/layouts.
     */
    virtual BindingLayoutHandle CreateBindingLayout(const BindingLayoutDesc& desc) = 0;
    virtual BindingLayoutHandle CreateBindlessLayout(const BindlessLayoutDesc& desc) = 0;
    virtual BindingSetHandle CreateBindingSet(const BindingSetDesc& desc, IBindingLayout* layout) = 0;
    virtual DescriptorTableHandle CreateDescriptorTable(IBindingLayout* layout) = 0;
    virtual void ResizeDescriptorTable(IDescriptorTable* descriptorTable, u32 newSize, bool keepContents = true) = 0;
    virtual bool WriteDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& item) = 0;

    /*
     * Command List.
     * XXX: Maybe have an internal "Job" handled by the RHI internally?
     * Something like:
     *      Receipt SubmitJob(Context context)
     * Having different contexts for each job type, eg. RenderContet, ComputContext, RayTracingContext would be great
     */
    virtual CommandListHandle CreateCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
    virtual u64 ExecuteCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists,
                                    CommandQueue executionQueue = CommandQueue::GRAPHICS)
        = 0;
    constexpr u64 ExecuteCommandList(ICommandList* commandList, CommandQueue executionQueue = CommandQueue::GRAPHICS)
    {
        return ExecuteCommandLists(&commandList, 1, executionQueue);
    }

    virtual void QueueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, u64 instance) = 0;
    virtual void WaitForIdle() = 0;

    /*
     * Present.
     */
    virtual void Present(ITexture* texture) = 0;

    /*
     * Misc.
     */

    // Cleanup: for now this is only used to retire unsued command buffers.
    virtual void Cleanup() = 0;

    virtual Object GetNativeQueue(ObjectType objectType, CommandQueue queue) = 0;

    virtual bool QueryFeatureSupport(RHIFeature feature, void* pInfo = nullptr, size_t infoSize = 0) = 0;
    virtual FormatSupport QueryFormatSupport(Format format) = 0;
};

using DeviceHandle = RefCountPtr<IDevice>;

template <class T> void hash_combine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace SuohRHI

/*
 * Custom hash functions.
 */
namespace std
{

template <typename T> struct hash<SuohRHI::RefCountPtr<T>>
{
    size_t operator()(SuohRHI::RefCountPtr<T> const& s) const noexcept
    {
        std::hash<T*> hash;
        return hash(s.Get());
    }
};

template <> struct hash<SuohRHI::TextureSubresourceSet>
{
    size_t operator()(SuohRHI::TextureSubresourceSet const& s) const noexcept
    {
        size_t hash = 0;
        SuohRHI::hash_combine(hash, s.baseMipLevel);
        SuohRHI::hash_combine(hash, s.numMipLevels);
        SuohRHI::hash_combine(hash, s.baseArrayLayer);
        SuohRHI::hash_combine(hash, s.numArrayLayers);
        return hash;
    }
};

template <> struct hash<SuohRHI::BufferRange>
{
    size_t operator()(SuohRHI::BufferRange const& s) const noexcept
    {
        size_t hash = 0;
        SuohRHI::hash_combine(hash, s.byteOffset);
        SuohRHI::hash_combine(hash, s.byteSize);
        return hash;
    }
};

template <> struct hash<SuohRHI::BindingSetItem>
{
    size_t operator()(SuohRHI::BindingSetItem const& s) const noexcept
    {
        size_t value = 0;
        SuohRHI::hash_combine(value, s.resourceHandle);
        SuohRHI::hash_combine(value, s.slot);
        SuohRHI::hash_combine(value, s.type);
        SuohRHI::hash_combine(value, s.dimension);
        SuohRHI::hash_combine(value, s.format);
        // SuohRHI::hash_combine(value, s.rawData[0]);
        // SuohRHI::hash_combine(value, s.rawData[1]);
        return value;
    }
};

template <> struct hash<SuohRHI::BindingSetDesc>
{
    size_t operator()(SuohRHI::BindingSetDesc const& s) const noexcept
    {
        size_t value = 0;
        for (const auto& item : s.bindings)
            hash_combine(value, item);
        return value;
    }
};

template <> struct hash<SuohRHI::FramebufferInfo>
{
    size_t operator()(SuohRHI::FramebufferInfo const& s) const noexcept
    {
        size_t hash = 0;
        for (auto format : s.colorFormats)
            SuohRHI::hash_combine(hash, format);
        SuohRHI::hash_combine(hash, s.depthFormat);
        SuohRHI::hash_combine(hash, s.width);
        SuohRHI::hash_combine(hash, s.height);
        SuohRHI::hash_combine(hash, s.sampleCount);
        SuohRHI::hash_combine(hash, s.sampleQuality);
        return hash;
    }
};

template <> struct hash<SuohRHI::RTBlendState>
{
    size_t operator()(SuohRHI::RTBlendState const& s) const noexcept
    {
        size_t hash = 0;
        SuohRHI::hash_combine(hash, s.blendEnable);
        SuohRHI::hash_combine(hash, s.srcBlend);
        SuohRHI::hash_combine(hash, s.destBlend);
        SuohRHI::hash_combine(hash, s.blendOp);
        SuohRHI::hash_combine(hash, s.srcBlendAlpha);
        SuohRHI::hash_combine(hash, s.destBlendAlpha);
        SuohRHI::hash_combine(hash, s.blendOpAlpha);
        SuohRHI::hash_combine(hash, s.renderTargetWriteMask);
        return hash;
    }
};

template <> struct hash<SuohRHI::BlendState>
{
    size_t operator()(SuohRHI::BlendState const& s) const noexcept
    {
        size_t hash = 0;
        SuohRHI::hash_combine(hash, s.alphaToCoverageEnable);
        for (const auto& target : s.renderTargets)
            SuohRHI::hash_combine(hash, target);
        return hash;
    }
};

} // namespace std
