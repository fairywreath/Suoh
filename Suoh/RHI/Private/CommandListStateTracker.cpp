#include "CommandListStateTracker.h"

namespace SuohRHI
{

namespace
{

u32 CalculateSubresource(MipLevel mipLevel, ArrayLayer arrayLayer, const TextureDesc& desc)
{
    return mipLevel + arrayLayer * desc.mipLevels;
}

} // namespace

bool VerifyPermanentResourceState(ResourceStates permanentState, ResourceStates requiredState)
{
    return ((permanentState & requiredState) == requiredState);
}

void CommandListResourceStateTracker::SetEnableSSBOBarriersForTexture(TextureStateTracker* texture, bool enableBarriers)
{
    TextureState* tracking = GetTextureStateTracking(texture, true);

    tracking->enableSSBOBarriers = enableBarriers;
    tracking->firstSSBOBarrierPlaced = false;
}

void CommandListResourceStateTracker::SetEnableSSBOBarriersForBuffer(BufferStateTracker* buffer, bool enableBarriers)
{
    BufferState* tracking = GetBufferStateTracking(buffer, true);

    tracking->enableSSBOBarriers = enableBarriers;
    tracking->firstSSBOBarrierPlaced = false;
}

void CommandListResourceStateTracker::BeginTrackingTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources,
                                                                ResourceStates stateBits)
{
    const TextureDesc& desc = texture->descRef;

    TextureState* tracking = GetTextureStateTracking(texture, true);

    subresources = subresources.Resolve(desc, false);

    if (subresources.IsEntireTexture(desc))
    {
        tracking->state = stateBits;
        tracking->subresourceStates.clear();
    }
    else
    {
        tracking->subresourceStates.resize(desc.mipLevels * desc.layerCount, tracking->state);
        tracking->state = ResourceStates::UNKNOWN;

        for (MipLevel mipLevel = subresources.baseMipLevel; mipLevel < subresources.baseMipLevel + subresources.numMipLevels; mipLevel++)
        {
            for (ArrayLayer arrayLayer = subresources.baseArrayLayer;
                 arrayLayer < subresources.baseArrayLayer + subresources.numArrayLayers; arrayLayer++)
            {
                u32 subresource = CalculateSubresource(mipLevel, arrayLayer, desc);
                tracking->subresourceStates[subresource] = stateBits;
            }
        }
    }
}

void CommandListResourceStateTracker::BeginTrackingBufferState(BufferStateTracker* buffer, ResourceStates stateBits)
{
    BufferState* tracking = GetBufferStateTracking(buffer, true);

    tracking->state = stateBits;
}

void CommandListResourceStateTracker::EndTrackingTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources,
                                                              ResourceStates stateBits, bool permanent)
{
    const TextureDesc& desc = texture->descRef;

    subresources = subresources.Resolve(desc, false);

    if (permanent && !subresources.IsEntireTexture(desc))
    {
        permanent = false;
    }

    RequireTextureState(texture, subresources, stateBits);

    if (permanent)
    {
        m_PermanentTextureStates.push_back(std::make_pair(texture, stateBits));
        GetTextureStateTracking(texture, true)->permanentTransition = true;
    }
}

void CommandListResourceStateTracker::EndTrackingBufferState(BufferStateTracker* buffer, ResourceStates stateBits, bool permanent)
{
    RequireBufferState(buffer, stateBits);

    if (permanent)
    {
        m_PermanentBufferStates.push_back(std::make_pair(buffer, stateBits));
    }
}

ResourceStates CommandListResourceStateTracker::GetTextureSubresourceState(TextureStateTracker* texture, ArrayLayer arrayLayer,
                                                                           MipLevel mipLevel)
{
    TextureState* tracking = GetTextureStateTracking(texture, false);

    if (!tracking)
        return ResourceStates::UNKNOWN;

    u32 subresource = CalculateSubresource(mipLevel, arrayLayer, texture->descRef);
    return tracking->subresourceStates[subresource];
}

ResourceStates CommandListResourceStateTracker::GetBufferState(BufferStateTracker* buffer)
{
    BufferState* tracking = GetBufferStateTracking(buffer, false);

    if (!tracking)
        return ResourceStates::UNKNOWN;

    return tracking->state;
}

void CommandListResourceStateTracker::RequireTextureState(TextureStateTracker* texture, TextureSubresourceSet subresources,
                                                          ResourceStates state)
{
    if (texture->permanentState != 0)
    {
        // VerifyPermanentResourceState(texture->permanentState, state);
        return;
    }

    subresources = subresources.Resolve(texture->descRef, false);

    TextureState* tracking = GetTextureStateTracking(texture, true);

    if (subresources.IsEntireTexture(texture->descRef) && tracking->subresourceStates.empty())
    {

        bool transitionNecessary = tracking->state != state;
        bool ssboNecessary
            = ((state & ResourceStates::UNORDERED_ACCESS) != 0) && (tracking->enableSSBOBarriers || !tracking->firstSSBOBarrierPlaced);

        if (transitionNecessary || ssboNecessary)
        {
            TextureBarrier barrier;
            barrier.texture = texture;
            barrier.entireTexture = true;
            barrier.stateBefore = tracking->state;
            barrier.stateAfter = state;
            m_TextureBarriers.push_back(barrier);
        }

        tracking->state = state;

        if (ssboNecessary && !transitionNecessary)
        {
            tracking->firstSSBOBarrierPlaced = true;
        }
    }
    else
    {

        bool stateExpanded = false;
        if (tracking->subresourceStates.empty())
        {

            tracking->subresourceStates.resize(texture->descRef.mipLevels * texture->descRef.layerCount, tracking->state);
            tracking->state = ResourceStates::UNKNOWN;
            stateExpanded = true;
        }

        bool anyUavBarrier = false;

        for (ArrayLayer arrayLayer = subresources.baseArrayLayer; arrayLayer < subresources.baseArrayLayer + subresources.numArrayLayers;
             arrayLayer++)
        {
            for (MipLevel mipLevel = subresources.baseMipLevel; mipLevel < subresources.baseMipLevel + subresources.numMipLevels;
                 mipLevel++)
            {
                u32 subresourceIndex = CalculateSubresource(mipLevel, arrayLayer, texture->descRef);

                auto priorState = tracking->subresourceStates[subresourceIndex];

                bool transitionNecessary = priorState != state;
                bool ssboNecessary = ((state & ResourceStates::UNORDERED_ACCESS) != 0) && !anyUavBarrier
                                     && (tracking->enableSSBOBarriers || !tracking->firstSSBOBarrierPlaced);

                if (transitionNecessary || ssboNecessary)
                {
                    TextureBarrier barrier;
                    barrier.texture = texture;
                    barrier.entireTexture = false;
                    barrier.mipLevel = mipLevel;
                    barrier.arrayLayer = arrayLayer;
                    barrier.stateBefore = priorState;
                    barrier.stateAfter = state;
                    m_TextureBarriers.push_back(barrier);
                }

                tracking->subresourceStates[subresourceIndex] = state;

                if (ssboNecessary && !transitionNecessary)
                {
                    anyUavBarrier = true;
                    tracking->firstSSBOBarrierPlaced = true;
                }
            }
        }
    }
}

void CommandListResourceStateTracker::RequireBufferState(BufferStateTracker* buffer, ResourceStates state)
{
    if (buffer->descRef.isVolatile)
        return;

    if (buffer->permanentState != 0)
    {
        // VerifyPermanentResourceState(texture->permanentState, state);
        return;
    }

    if (buffer->descRef.cpuAccess != CpuAccessMode::NONE)
    {
        // CPU mapped/visible buffers cannot change state.
        return;
    }

    BufferState* tracking = GetBufferStateTracking(buffer, true);

    bool transitionNecessary = tracking->state != state;
    bool ssboNecessary
        = ((state & ResourceStates::UNORDERED_ACCESS) != 0) && (tracking->enableSSBOBarriers || !tracking->firstSSBOBarrierPlaced);

    if (transitionNecessary)
    {
        // See if this buffer is already used for a different purpose in this batch.
        // If it is, combine the state bits.
        for (BufferBarrier& barrier : m_BufferBarriers)
        {
            if (barrier.buffer == buffer)
            {
                barrier.stateAfter = ResourceStates(barrier.stateAfter | state);
                tracking->state = barrier.stateAfter;
                return;
            }
        }
    }

    if (transitionNecessary || ssboNecessary)
    {
        BufferBarrier barrier;
        barrier.buffer = buffer;
        barrier.stateBefore = tracking->state;
        barrier.stateAfter = state;
        m_BufferBarriers.push_back(barrier);
    }

    if (ssboNecessary && !transitionNecessary)
    {
        tracking->firstSSBOBarrierPlaced = true;
    }

    tracking->state = state;
}

void CommandListResourceStateTracker::KeepBufferInitialStates()
{
    for (auto& [buffer, tracking] : m_BufferStates)
    {
        if (buffer->descRef.keepInitialState && !buffer->permanentState && !buffer->descRef.isVolatile && !tracking->permanentTransition)
        {
            RequireBufferState(buffer, buffer->descRef.initialState);
        }
    }
}

void CommandListResourceStateTracker::KeepTextureInitialStates()
{
    for (auto& [texture, tracking] : m_TextureStates)
    {
        if (texture->descRef.keepInitialState && !texture->permanentState && !tracking->permanentTransition)
        {
            RequireTextureState(texture, AllSubresources, texture->descRef.initialState);
        }
    }
}

void CommandListResourceStateTracker::CommandListSubmitted()
{
    for (auto [texture, state] : m_PermanentTextureStates)
    {
        if (texture->permanentState != 0 && texture->permanentState != state)
            continue;

        texture->permanentState = state;
    }
    m_PermanentTextureStates.clear();

    for (auto [buffer, state] : m_PermanentBufferStates)
    {
        if (buffer->permanentState != 0 && buffer->permanentState != state)
            continue;

        buffer->permanentState = state;
    }

    m_PermanentBufferStates.clear();

    for (const auto& [texture, stateTracking] : m_TextureStates)
    {
        if (texture->descRef.keepInitialState && !texture->stateInitialized)
            texture->stateInitialized = true;
    }

    m_TextureStates.clear();
    m_BufferStates.clear();
}

TextureState* CommandListResourceStateTracker::GetTextureStateTracking(TextureStateTracker* texture, bool allowCreate)
{
    auto it = m_TextureStates.find(texture);

    if (it != m_TextureStates.end())
    {
        return it->second.get();
    }

    if (!allowCreate)
        return nullptr;

    std::unique_ptr<TextureState> trackingRef = std::make_unique<TextureState>();

    TextureState* tracking = trackingRef.get();
    m_TextureStates.insert(std::make_pair(texture, std::move(trackingRef)));

    if (texture->descRef.keepInitialState)
    {
        tracking->state = texture->stateInitialized ? texture->descRef.initialState : ResourceStates::COMMON;
    }

    return tracking;
}

BufferState* CommandListResourceStateTracker::GetBufferStateTracking(BufferStateTracker* buffer, bool allowCreate)
{
    auto it = m_BufferStates.find(buffer);

    if (it != m_BufferStates.end())
    {
        return it->second.get();
    }

    if (!allowCreate)
        return nullptr;

    std::unique_ptr<BufferState> trackingRef = std::make_unique<BufferState>();

    BufferState* tracking = trackingRef.get();
    m_BufferStates.insert(std::make_pair(buffer, std::move(trackingRef)));

    if (buffer->descRef.keepInitialState)
    {
        tracking->state = buffer->descRef.initialState;
    }

    return tracking;
}

} // namespace SuohRHI
