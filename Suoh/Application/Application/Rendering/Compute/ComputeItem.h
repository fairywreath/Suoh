#pragma once

#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>

namespace Suoh
{

/*
 * Class that represents a GPU computed resource.
 */
class ComputeItem
{
public:
    ComputeItem(VKRenderDevice* renderDevice, u32 uniformBufferSize);
    virtual ~ComputeItem();

    void recordCommands(void* pushConstant = nullptr, u32 pushConstantSize = 0, u32 xsize = 1, u32 ysize = 1, u32 zsize = 1);
    bool submit();

    inline void uploadUniformBuffer(void* data, u32 size)
    {
        mRenderDevice->uploadBufferData(mUniformBuffer, data, size);
    }

private:
    void waitFence();

protected:
    VKRenderDevice* mRenderDevice;

    // XXX: how to efficiently abstract this synchronization object?
    VkFence mFence;

    Buffer mUniformBuffer;

    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
};

} // namespace Suoh
