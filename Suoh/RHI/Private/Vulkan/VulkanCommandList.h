#pragma once

#include "VulkanCommon.h"

#include "VulkanBindings.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanQueue.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanUploadManager.h"
#include "VulkanUtils.h"

#include "CommandListStateTracker.h"

namespace SuohRHI
{

namespace Vulkan
{

class VulkanDevice;

class VulkanCommandList : public RefCountResource<ICommandList>
{
public:
    VulkanCommandList(VulkanDevice* device, const VulkanContext& context, const CommandListParameters& parameters);
    ~VulkanCommandList() override;

    virtual IDevice* GetDevice() override;
    virtual const CommandListParameters& GetDesc() const override
    {
        return m_CommandListParameters;
    }

    virtual Object GetNativeObject(ObjectType type) override;

    virtual void Begin() override;
    virtual void End() override;

    virtual void ClearState() override;
    virtual void ClearTexture(ITexture* texture, TextureSubresourceSet subresources, const Color& clearColor) override;
    virtual void ClearTextureUint(ITexture* texture, TextureSubresourceSet subresources, u32 clearColor) override;
    virtual void ClearDepthStencilTexture(ITexture* texture, TextureSubresourceSet subresources, bool clearDepth, float depth,
                                          bool clearStencil, u8 stencil) override;

    virtual void CopyTexture(ITexture* dest, const TextureLayer& destLayer, ITexture* src, const TextureLayer* srcLayer) override;
    virtual void CopyTexture(IStagingTexture* dest, const TextureLayer& destLayer, ITexture* src, const TextureLayer& srcLayer) override;
    virtual void CopyTexture(ITexture* dest, const TextureLayer& destLayer, IStagingTexture* src, const TextureLayer& srcLayer) override;
    virtual void WriteTexture(ITexture* dest, u32 arrayLayer, u32 mipLevel, const void* data, size_t rowPitch,
                              size_t depthPitch = 0) override;
    virtual void ResolveTexture(ITexture* dest, const TextureSubresourceSet& dstSubresources, ITexture* src,
                                const TextureSubresourceSet& srcSubresources) override;

    virtual void WriteBuffer(IBuffer* b, const void* data, size_t dataSize, u64 destOffsetBytes = 0) override;
    virtual void ClearBufferUInt(IBuffer* b, u32 clearValue) override;
    virtual void CopyBuffer(IBuffer* dest, u64 destOffsetBytes, IBuffer* src, u64 srcOffsetBytes, u64 dataSizeBytes) override;

    virtual void SetPushConstants(const void* data, size_t byteSize) override;

    virtual void SetGraphicsState(const GraphicsState& state) override;

    virtual void Draw(const DrawArguments& args) override;
    virtual void DrawIndexed(const DrawArguments& args) override;

    // XXX: Properly implemenent these.
    virtual void DrawIndirect(u32 offsetBytes) override;
    virtual void DrawIndirectCount(u32 offsetBytes, u32 drawCount) override;
    // XXX: DrawIndexedIndirect, etc...

    virtual void SetEnableAutomaticBarriers(bool enable) override;
    virtual void SetResourceStatesForBindingSet(IBindingSet* bindingSet) override;

    virtual void BeginTrackingTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) override;
    virtual void BeginTrackingBufferState(IBuffer* buffer, ResourceStates stateBits) override;

    virtual void SetTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) override;
    virtual void SetBufferState(IBuffer* buffer, ResourceStates stateBits) override;
    virtual void SetPermanentTextureState(ITexture* texture, ResourceStates stateBits) override;
    virtual void SetPermanentBufferState(IBuffer* buffer, ResourceStates stateBits) override;

    virtual void SetEnableSSBOBarriersForTexture(ITexture* texture, bool enableBarriers) override;
    virtual void SetEnableSSBOBarriersForBuffer(IBuffer* buffer, bool enableBarriers) override;

    virtual void CommitBarriers() override;

    virtual ResourceStates GetTextureSubresourceState(ITexture* texture, ArrayLayer arrayLayer, MipLevel mipLevel) override;
    virtual ResourceStates GetBufferState(IBuffer* buffer) override;

    // XXX: Compute pipeline not implemented yet.
    virtual void SetComputeState(const ComputeState& state) override;
    virtual void Dispatch(u32 groupsX, u32 groupsY = 1, u32 groupsZ = 1) override;
    virtual void DispatchIndirect(u32 offsetBytes) override;

    // XXX: Query events not implemented yet.
    virtual void BeginTimerQuery(ITimerQuery* query) override;
    virtual void EndTimerQuery(ITimerQuery* query) override;
    virtual void BeginMarker(const char* name) override;
    virtual void EndMarker() override;

    /*
     * VulkanCommandList specific.
     */
    // Reset state of currentCommandBuffer.
    void MarkExecuted(VulkanQueue& queue, u64 submissionID);
    TrackedCommandBufferPtr GetCurrentCommandBuffer()
    {
        return m_CurrentCommandBuffer;
    }

private:
    void BindBindingSets(const BindingSetVector& bindings, vk::PipelineLayout pipelineLayout, vk::PipelineBindPoint bindPoint);
    void ClearTexture(ITexture* texture, TextureSubresourceSet subresources, const vk::ClearColorValue& clearColorValue);
    void EndRenderPass();

    void TrackResourcesAndBarriers(const GraphicsState& state);

    void RequireTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates state);
    void RequireBufferState(IBuffer* buffer, ResourceStates state);
    bool AnyBarriers() const;

private:
    VulkanDevice* m_pDevice;
    const VulkanContext& m_Context;
    CommandListParameters m_CommandListParameters;

    TrackedCommandBufferPtr m_CurrentCommandBuffer;

    // State tracking and barrier management.
    CommandListResourceStateTracker m_ResourceStateTracker;
    bool m_EnableAutomaticBarriers{true};

    // Graphics pipeline state.
    vk::PipelineLayout m_CurrentPipelineLayout;
    vk::ShaderStageFlags m_CurrentPipelineShaderStages;
    GraphicsState m_CurrentGraphicsState{};

    // XXX: Manage volatile buffers?

    // Upload managers.
    std::unique_ptr<VulkanUploadManager> m_UploadManager;
    std::unique_ptr<VulkanUploadManager> m_ScratchManager;
};

} // namespace Vulkan

} // namespace SuohRHI
