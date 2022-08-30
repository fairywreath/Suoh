#pragma once

#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>

namespace Suoh
{

class ComputeBase
{
public:
    ComputeBase(VKRenderDevice* renderDevice, const std::string& shaderFile, u32 inputSize, u32 outputSize);
    virtual ~ComputeBase();

    inline void uploadInput(u32 offset, void* inData, u32 byteCount)
    {
        mRenderDevice->uploadBufferData(mInBuffer, inData, offset, byteCount);
    }

    inline void downloadOutput(u32 offset, void* outData, u32 byteCount)
    {
        mRenderDevice->downloadBuferData(mOutBuffer, outData, offset, byteCount);
    }

    inline bool execute(u32 xsize, u32 ysize, u32 zsize)
    {
        return mRenderDevice->executeComputeShader(mPipeline, mPipelineLayout, mDescriptorSet, xsize, ysize, zsize);
    }

protected:
    bool createComputeDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout);
    bool createComputeDescriptorSet(VkDescriptorSetLayout descriptorSetLayout);

    VKRenderDevice* mRenderDevice;

    Buffer mInBuffer;
    Buffer mOutBuffer;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;

    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSet mDescriptorSet;
};

} // namespace Suoh
