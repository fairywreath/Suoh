#include "ComputeItem.h"

namespace Suoh
{

ComputeItem::ComputeItem(VKRenderDevice* renderDevice, u32 uniformBufferSize)
    : mRenderDevice(renderDevice)
{
    mRenderDevice->createBuffer(uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mUniformBuffer);
    mRenderDevice->createFence(mFence, VK_FENCE_CREATE_SIGNALED_BIT);
}

ComputeItem::~ComputeItem()
{
    mRenderDevice->destroyFence(mFence);
    mRenderDevice->destroyBuffer(mUniformBuffer);
}

void ComputeItem::recordCommands(void* pushConstant, u32 pushConstantSize, u32 xsize, u32 ysize, u32 zsize)
{
    VkCommandBuffer commandBuffer = mRenderDevice->getComputeCommendBuffer();

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &mDescriptorSet, 0, 0);

    if (pushConstant && pushConstantSize > 0)
    {
        vkCmdPushConstants(commandBuffer, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, pushConstantSize, pushConstant);
    }

    vkCmdDispatch(commandBuffer, xsize, ysize, zsize);
    vkEndCommandBuffer(commandBuffer);
}

bool ComputeItem::submit()
{
    waitFence();
    return mRenderDevice->submitCompute(mRenderDevice->getComputeCommendBuffer(), mFence);
}

void ComputeItem::waitFence()
{
    mRenderDevice->waitFence(mFence);
}

} // namespace Suoh
