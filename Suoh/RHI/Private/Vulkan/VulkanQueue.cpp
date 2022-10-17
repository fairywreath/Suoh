#include "VulkanQueue.h"

#include "VulkanCommandList.h"

namespace SuohRHI
{

namespace Vulkan
{

SuohRHI::Vulkan::VulkanTrackedCommandBuffer::~VulkanTrackedCommandBuffer()
{
    m_Context.device.destroyCommandPool(commandPool);
}

VulkanQueue::VulkanQueue(const VulkanContext& context, vk::Queue queue, CommandQueue id, u32 queueFamilyIndex)
    : m_Context(context), m_Queue(queue), m_QueueID(id), m_QueueFamilyIndex(queueFamilyIndex)
{
    auto semaphoreTypeInfo = vk::SemaphoreTypeCreateInfo().setSemaphoreType(vk::SemaphoreType::eTimeline);
    auto semaphoreInfo = vk::SemaphoreCreateInfo().setPNext(&semaphoreTypeInfo);

    trackingSemaphore = m_Context.device.createSemaphore(semaphoreInfo);
}

VulkanQueue::~VulkanQueue()
{
    m_Context.device.destroySemaphore(trackingSemaphore);
    trackingSemaphore = vk::Semaphore();
}

TrackedCommandBufferPtr VulkanQueue::CreateCommandBuffer()
{
    TrackedCommandBufferPtr cmdBufPtr = std::make_shared<VulkanTrackedCommandBuffer>(m_Context);

    auto commandPoolInfo = vk::CommandPoolCreateInfo()
                               .setQueueFamilyIndex(m_QueueFamilyIndex)
                               .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);

    auto res = m_Context.device.createCommandPool(&commandPoolInfo, nullptr, &cmdBufPtr->commandPool);
    VK_CHECK_RETURN_NULL(res);

    auto allocInfo = vk::CommandBufferAllocateInfo()
                         .setLevel(vk::CommandBufferLevel::ePrimary)
                         .setCommandPool(cmdBufPtr->commandPool)
                         .setCommandBufferCount(1);

    res = m_Context.device.allocateCommandBuffers(&allocInfo, &cmdBufPtr->commandBuffer);
    VK_CHECK_RETURN_NULL(res);

    return cmdBufPtr;
}

TrackedCommandBufferPtr VulkanQueue::GetOrCreateCommandBuffer()
{
    std::lock_guard lockGuard(m_Mutex);

    u64 recordingID = ++m_LastRecordingID;

    TrackedCommandBufferPtr cmdBufPtr;
    if (m_CommandBuffersPool.empty())
    {
        cmdBufPtr = CreateCommandBuffer();
    }
    else
    {
        cmdBufPtr = m_CommandBuffersPool.front();
        m_CommandBuffersPool.pop_front();
    }

    cmdBufPtr->recordingID = recordingID;
    return cmdBufPtr;
}

u64 VulkanQueue::Submit(ICommandList* const* ppCommandList, size_t numCommandLists)
{
    std::vector<vk::PipelineStageFlags> waitStages(m_WaitSemaphores.size(), vk::PipelineStageFlagBits::eTopOfPipe);
    std::vector<vk::CommandBuffer> commandBuffers(numCommandLists);

    m_LastSubmittedID++;

    for (size_t i = 0; i < numCommandLists; i++)
    {
        VulkanCommandList* commandList = checked_cast<VulkanCommandList*>(ppCommandList[i]);
        auto commandBuffer = commandList->GetCurrentCommandBuffer();

        commandBuffers[i] = commandBuffer->commandBuffer;

        m_CommandBuffersInFlight.push_back(commandBuffer);

        for (const auto& buffer : commandBuffer->referencedStagingBuffers)
        {
            buffer->lastUseQueue = m_QueueID;
            buffer->lsatUseCommandListID = m_LastSubmittedID;
        }
    }

    m_SignalSemaphores.push_back(trackingSemaphore);
    m_SignalSemaphoreValues.push_back(m_LastSubmittedID);

    auto timelineSemInfo = vk::TimelineSemaphoreSubmitInfo()
                               .setSignalSemaphoreValueCount((u32)m_SignalSemaphoreValues.size())
                               .setPSignalSemaphoreValues(m_SignalSemaphoreValues.data());

    if (!m_WaitSemaphoreValues.empty())
    {
        timelineSemInfo.setWaitSemaphoreValueCount(m_WaitSemaphoreValues.size());
        timelineSemInfo.setPWaitSemaphoreValues(m_WaitSemaphoreValues.data());
    }

    auto submitInfo = vk::SubmitInfo()
                          .setPNext(&timelineSemInfo)
                          .setCommandBufferCount(u32(numCommandLists))
                          .setPCommandBuffers(commandBuffers.data())
                          .setWaitSemaphoreCount(u32(m_WaitSemaphores.size()))
                          .setPWaitSemaphores(m_WaitSemaphores.data())
                          .setPWaitDstStageMask(waitStages.data())
                          .setSignalSemaphoreCount(u32(m_SignalSemaphores.size()))
                          .setPSignalSemaphores(m_SignalSemaphores.data());

    m_Queue.submit(submitInfo);

    m_WaitSemaphores.clear();
    m_WaitSemaphoreValues.clear();
    m_SignalSemaphores.clear();
    m_SignalSemaphoreValues.clear();

    return m_LastSubmittedID;
}

void VulkanQueue::AddWaitSemaphore(vk::Semaphore semaphore, u32 value)
{
    if (!semaphore)
        return;

    m_WaitSemaphores.push_back(semaphore);
    m_WaitSemaphoreValues.push_back(value);
}

void VulkanQueue::AddSignalSemaphore(vk::Semaphore semaphore, u32 value)
{
    if (!semaphore)
        return;

    m_SignalSemaphores.push_back(semaphore);
    m_SignalSemaphoreValues.push_back(value);
}

void VulkanQueue::RetireCommandBuffers()
{
    auto cmdBuffers = std::move(m_CommandBuffersInFlight);
    m_CommandBuffersInFlight.clear();

    u64 lastFinishedID = UpdateLastFinishedID();

    for (const auto& cmd : cmdBuffers)
    {
        // Retire to reuse.
        if (cmd->submissionID <= lastFinishedID)
        {
            cmd->referencedResources.clear();
            cmd->referencedStagingBuffers.clear();
            cmd->submissionID = 0;
            m_CommandBuffersPool.push_back(cmd);
        }
        else
        {
            m_CommandBuffersInFlight.push_back(cmd);
        }
    }
}

TrackedCommandBufferPtr VulkanQueue::GetCommandBufferInFlight(u64 submissionID)
{
    for (const auto& cmd : m_CommandBuffersInFlight)
    {
        if (cmd->submissionID == submissionID)
            return cmd;
    }

    return nullptr;
}

u64 VulkanQueue::UpdateLastFinishedID()
{
    m_LastFinishedID = m_Context.device.getSemaphoreCounterValue(trackingSemaphore);

    return m_LastFinishedID;
}

bool VulkanQueue::PollCommandList(u64 commandListID)
{
    if (commandListID > m_LastSubmittedID || commandListID == 0)
        return false;

    bool completed = GetLastFinishedID() >= commandListID;
    if (completed)
        return true;

    completed = UpdateLastFinishedID() >= commandListID;
    return completed;
}

bool VulkanQueue::WaitCommandList(u64 commandListID, u64 timeout)
{
    if (commandListID > m_LastSubmittedID || commandListID == 0)
        return false;

    if (PollCommandList(commandListID))
        return true;

    std::array<const vk::Semaphore, 1> semaphores = {trackingSemaphore};
    std::array<u64, 1> waitValues = {commandListID};

    auto waitInfo = vk::SemaphoreWaitInfo().setSemaphores(semaphores).setValues(waitValues);

    // Commandlist ID will be signaled by the tracking semaphore or an external signal semaphore.
    vk::Result result = m_Context.device.waitSemaphores(waitInfo, timeout);

    return (result == vk::Result::eSuccess);
}

} // namespace Vulkan

} // namespace SuohRHI