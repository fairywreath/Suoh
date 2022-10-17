#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"

#include <list>

namespace SuohRHI
{

namespace Vulkan
{

class VulkanTrackedCommandBuffer
{
public:
    explicit VulkanTrackedCommandBuffer(const VulkanContext& context) : m_Context(context){};
    ~VulkanTrackedCommandBuffer();

    vk::CommandBuffer commandBuffer{};
    vk::CommandPool commandPool{};

    // resource tracking
    std::vector<ResourceHandle> referencedResources;
    std::vector<RefCountPtr<VulkanBuffer>> referencedStagingBuffers;

    u32 recordingID{0};
    u32 submissionID{0};

private:
    const VulkanContext& m_Context;
};

using TrackedCommandBufferPtr = std::shared_ptr<VulkanTrackedCommandBuffer>;

class VulkanQueue
{
public:
    VulkanQueue(const VulkanContext& context, vk::Queue queue, CommandQueue id, u32 queueFamilyIndex);
    ~VulkanQueue();

    TrackedCommandBufferPtr CreateCommandBuffer();
    TrackedCommandBufferPtr GetOrCreateCommandBuffer();

    u64 Submit(ICommandList* const* ppCommandList, size_t numCommandLists);

    void AddWaitSemaphore(vk::Semaphore semaphore, u32 value);
    void AddSignalSemaphore(vk::Semaphore semaphore, u32 value);

    void RetireCommandBuffers();
    TrackedCommandBufferPtr GetCommandBufferInFlight(u64 submissionID);

    u64 UpdateLastFinishedID();

    u64 GetLastSubmittedID() const
    {
        return m_LastSubmittedID;
    }
    u64 GetLastFinishedID() const
    {
        return m_LastFinishedID;
    }
    vk::Queue GetVkQueue() const
    {
        return m_Queue;
    }
    CommandQueue GetQueueID() const
    {
        return m_QueueID;
    }

    bool PollCommandList(u64 commandListID);
    bool WaitCommandList(u64 commandListID, u64 timeout);

    // Timeline/coutning semaphore to track unique submission IDs.
    vk::Semaphore trackingSemaphore;

private:
    const VulkanContext& m_Context;

    vk::Queue m_Queue;
    CommandQueue m_QueueID;
    u32 m_QueueFamilyIndex{};

    // Guard GetOrCreateCommandBuffer since m_CommandBuffersInFlight is accessed.
    std::mutex m_Mutex;

    // Timeline semaphores.
    std::vector<vk::Semaphore> m_WaitSemaphores;
    std::vector<u64> m_WaitSemaphoreValues;
    std::vector<vk::Semaphore> m_SignalSemaphores;
    std::vector<u64> m_SignalSemaphoreValues;

    // Retired command buffer to reuse.
    std::list<TrackedCommandBufferPtr> m_CommandBuffersPool;

    // Currently processed/submitted command buffers.
    std::list<TrackedCommandBufferPtr> m_CommandBuffersInFlight;

    // Counter value for GetOrCreateCommandBuffer.
    u64 m_LastRecordingID{0};

    // Counter value given to tracking(timeline) semaphore during submission.
    u64 m_LastSubmittedID{0};

    // Counter value taken from tracking(timeline) semaphore, based on lastSubmittedID.
    u64 m_LastFinishedID{0};
};

} // namespace Vulkan

} // namespace SuohRHI
