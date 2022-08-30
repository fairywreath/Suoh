#include "ComputeBase.h"

namespace Suoh
{

ComputeBase::ComputeBase(VKRenderDevice* renderDevice, const std::string& shaderFile, u32 inputSize, u32 outputSize)
    : mRenderDevice(renderDevice)
{
    mRenderDevice->createSharedBuffer(inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mInBuffer);
    mRenderDevice->createSharedBuffer(inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, mOutBuffer);

    ShaderModule shader;
    mRenderDevice->createShader(shaderFile, shader);

    createComputeDescriptorSetLayout(mDescriptorSetLayout);

    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createComputePipeline(mPipeline, mPipelineLayout, shader.shaderModule);

    createComputeDescriptorSet(mDescriptorSetLayout);

    mRenderDevice->destroyShader(shader);
}

ComputeBase::~ComputeBase()
{
    mRenderDevice->destroyBuffer(mInBuffer);
    mRenderDevice->destroyBuffer(mOutBuffer);

    mRenderDevice->destroyPipeline(mPipeline);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);
}

bool ComputeBase::createComputeDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    mRenderDevice->createDescriptorSetLayout(bindings, descriptorSetLayout);

    return true;
}

bool ComputeBase::createComputeDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
    VkDescriptorPoolSize descriptorPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 2,
    };

    const VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize,
    };

    mRenderDevice->createDescriptorPool(descPoolInfo, mDescriptorPool);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        0,
        mDescriptorPool,
        1,
        &descriptorSetLayout,
    };

    const auto device = mRenderDevice->getVkDevice();
    VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &mDescriptorSet));

    VkDescriptorBufferInfo inBufferInfo = {
        mInBuffer.buffer,
        0,
        VK_WHOLE_SIZE,
    };

    VkDescriptorBufferInfo outBufferInfo = {
        mOutBuffer.buffer,
        0,
        VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet writeDescriptorSet[2] = {
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            0,
            mDescriptorSet,
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            0,
            &inBufferInfo,
            0,
        },
        {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            0,
            mDescriptorSet,
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            0,
            &outBufferInfo,
            0,
        },
    };

    vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);

    return true;
}

} // namespace Suoh
