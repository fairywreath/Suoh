#pragma once

#include "RHI.h"

#include <memory>
#include <unordered_map>

namespace SuohRHI
{

struct BufferStateTracker
{
    explicit BufferStateTracker(const BufferDesc& desc) : descRef(desc)
    {
    }

    const BufferDesc& descRef;
    ResourceStates permanentState{ResourceStates::UNKNOWN};
};

struct TextureStateTracker
{
    explicit TextureStateTracker(const TextureDesc& desc) : descRef(desc)
    {
    }

    const TextureDesc& descRef;
    ResourceStates permanentState{ResourceStates::UNKNOWN};
    bool stateInitialized{false};
};

struct TextureState
{
    std::vector<ResourceStates> subresourceStates;
    ResourceStates state{ResourceStates::UNKNOWN};

    bool permanentTransition{false};

    bool enableSSBOBarriers{true};
    bool firstSSBOBarrierPlaced{false};
};

struct BufferState
{
    ResourceStates state{ResourceStates::UNKNOWN};

    bool permanentTransition{false};

    bool enableSSBOBarriers{true};
    bool firstSSBOBarrierPlaced{false};
};

struct TextureBarrier
{
    TextureStateTracker* texture{nullptr};

    MipLevel mipLevel{0};
    ArrayLayer arrayLayer{0};

    bool entireTexture{false};

    ResourceStates stateBefore{ResourceStates::UNKNOWN};
    ResourceStates stateAfter{ResourceStates::UNKNOWN};
};

struct BufferBarrier
{
    BufferStateTracker* buffer{nullptr};

    ResourceStates stateBefore{ResourceStates::UNKNOWN};
    ResourceStates stateAfter{ResourceStates::UNKNOWN};
};

bool VerifyPermanentResourceState(ResourceStates permanentState, ResourceStates requiredState);

class CommandListResourceStateTracker
{
public:
    CommandListResourceStateTracker() = default;

    void SetEnableSSBOBarriersForTexture(TextureStateTracker* texture, bool enableBarriers);
    void SetEnableSSBOBarriersForBuffer(BufferStateTracker* buffer, bool enableBarriers);

    void BeginTrackingTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources, ResourceStates stateBits);
    void BeginTrackingBufferState(BufferStateTracker* buffer, ResourceStates stateBits);

    void EndTrackingTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources, ResourceStates stateBits,
                                 bool permanent);
    void EndTrackingBufferState(BufferStateTracker* buffer, ResourceStates stateBits, bool permanent);

    ResourceStates GetTextureSubresourceState(TextureStateTracker* texture, ArrayLayer arrayLayer, MipLevel mipLevel);
    ResourceStates GetBufferState(BufferStateTracker* buffer);

    void RequireTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources, ResourceStates state);
    void RequireBufferState(BufferStateTracker* buffer, ResourceStates state);

    void KeepBufferInitialStates();
    void KeepTextureInitialStates();
    void CommandListSubmitted();

    [[nodiscard]] const std::vector<TextureBarrier>& GetTextureBarriers() const
    {
        return m_TextureBarriers;
    }
    [[nodiscard]] const std::vector<BufferBarrier>& GetBufferBarriers() const
    {
        return m_BufferBarriers;
    }
    void ClearBarriers()
    {
        m_TextureBarriers.clear();
        m_BufferBarriers.clear();
    }

private:
    TextureState* GetTextureStateTracking(TextureStateTracker* texture, bool allowCreate);
    BufferState* GetBufferStateTracking(BufferStateTracker* buffer, bool allowCreate);

private:
    std::unordered_map<TextureStateTracker*, std::unique_ptr<TextureState>> m_TextureStates;
    std::unordered_map<BufferStateTracker*, std::unique_ptr<BufferState>> m_BufferStates;

    std::vector<std::pair<TextureStateTracker*, ResourceStates>> m_PermanentTextureStates;
    std::vector<std::pair<BufferStateTracker*, ResourceStates>> m_PermanentBufferStates;

    std::vector<TextureBarrier> m_TextureBarriers;
    std::vector<BufferBarrier> m_BufferBarriers;
};

} // namespace SuohRHI
