#include "VulkanBindings.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace RenderLib
{

namespace Vulkan
{

namespace
{

inline vk::DescriptorSetLayoutBinding MakeVkBinding(u32 binding, vk::DescriptorType type,
                                                    vk::ShaderStageFlags flags, u32 count = 1)
{
    return vk::DescriptorSetLayoutBinding()
        .setBinding(binding)
        .setDescriptorType(type)
        .setDescriptorCount(count)
        .setStageFlags(flags);
}

inline vk::DescriptorPoolSize MakeVkDescriptorPoolSize(vk::DescriptorType type, u32 count)
{
    return vk::DescriptorPoolSize().setType(type).setDescriptorCount(count);
}

inline vk::WriteDescriptorSet MakeVkWriteDescriptorSet(vk::DescriptorSet set,
                                                       const vk::DescriptorBufferInfo* bufferInfo,
                                                       const vk::DescriptorImageInfo* imageInfo,
                                                       u32 binding, vk::DescriptorType type,
                                                       u32 count = 1)
{
    return vk::WriteDescriptorSet()
        .setDstSet(set)
        .setDstBinding(binding)
        .setDstArrayElement(0)
        .setDescriptorCount(count)
        .setDescriptorType(type)
        .setPBufferInfo(bufferInfo)
        .setPImageInfo(imageInfo);
}

} // namespace

DescriptorLayoutHandle VulkanDevice::CreateDescriptorLayout(const DescriptorLayoutDesc& desc)
{
    auto handle = DescriptorLayoutHandle::Create(new DescriptorLayout(m_Context));
    handle->desc = desc;

    // Setup bindings.
    // "Automatically" figure out slots for now.
    u32 bindingIndex = 0;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.bufferDescriptors.size() + desc.imageArrayDescriptors.size()
                     + desc.imageDescriptors.size());

    for (auto& bd : handle->desc.bufferDescriptors)
    {
        bd.descriptorItem.bindingSlot = bindingIndex;
        bindings.push_back(MakeVkBinding(bindingIndex++, bd.descriptorItem.type,
                                         bd.descriptorItem.shaderStageFlags));
    }

    for (auto& id : handle->desc.imageDescriptors)
    {
        id.descriptorItem.bindingSlot = bindingIndex;
        bindings.push_back(MakeVkBinding(bindingIndex++, id.descriptorItem.type,
                                         id.descriptorItem.shaderStageFlags));
    }

    for (auto& iad : handle->desc.imageArrayDescriptors)
    {
        iad.descriptorItem.bindingSlot = bindingIndex;
        bindings.push_back(MakeVkBinding(bindingIndex++, vk::DescriptorType::eCombinedImageSampler,
                                         iad.descriptorItem.shaderStageFlags, iad.images.size()));
    }

    if (bindings.empty())
    {
        LOG_WARN("CreateDescriptorLayout: no bindings attached!");
    }

    const auto layoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(bindings);
    VK_CHECK_RETURN_NULL(m_Context.device.createDescriptorSetLayout(&layoutInfo, nullptr,
                                                                    &handle->descriptorSetLayout));
    handle->bindings = bindings;

    // Setup pool sizes.
    u32 uniformBufferCount = 0;
    u32 storageBufferCount = 0;
    u32 samplerCount = static_cast<u32>(desc.imageDescriptors.size());

    for (const auto& iad : desc.imageArrayDescriptors)
        samplerCount += static_cast<u32>(iad.images.size());

    for (const auto& bd : desc.bufferDescriptors)
    {
        if (bd.descriptorItem.type == vk::DescriptorType::eUniformBuffer)
            uniformBufferCount++;
        else if (bd.descriptorItem.type == vk::DescriptorType::eStorageBuffer)
            storageBufferCount++;
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;

    if (uniformBufferCount > 0)
    {
        poolSizes.push_back(MakeVkDescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer, uniformBufferCount * desc.poolCountMultiplier));
    }

    if (storageBufferCount > 0)
    {
        poolSizes.push_back(MakeVkDescriptorPoolSize(
            vk::DescriptorType::eStorageBuffer, storageBufferCount * desc.poolCountMultiplier));
    }

    if (samplerCount)
    {
        poolSizes.push_back(MakeVkDescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
                                                     samplerCount * desc.poolCountMultiplier));
    }

    const auto poolInfo = vk::DescriptorPoolCreateInfo()
                              .setMaxSets(desc.poolCountMultiplier)
                              .setPoolSizes(poolSizes);

    VK_CHECK_RETURN_NULL(
        m_Context.device.createDescriptorPool(&poolInfo, nullptr, &handle->descriptorPool));
    handle->poolSizes = poolSizes;

    return handle;
}

DescriptorLayout::~DescriptorLayout()
{
    if (descriptorSetLayout)
    {
        m_Context.device.destroyDescriptorSetLayout(descriptorSetLayout);
    }
    if (descriptorPool)
    {
        m_Context.device.destroyDescriptorPool(descriptorPool);
    }
}

DescriptorSetHandle VulkanDevice::CreateDescriptorSet(const DescriptorSetDesc& desc)
{
    auto handle = DescriptorSetHandle::Create(new DescriptorSet(m_Context));

    const auto setInfo = vk::DescriptorSetAllocateInfo()
                             .setDescriptorPool(desc.layout->descriptorPool)
                             .setDescriptorSetCount(1)
                             .setSetLayouts(desc.layout->descriptorSetLayout);
    VK_CHECK_RETURN_NULL(m_Context.device.allocateDescriptorSets(&setInfo, &handle->descriptorSet));
    handle->desc = desc;

    handle->UpdateDescriptorSets();

    return handle;
}

DescriptorSet::~DescriptorSet()
{
    // Pool should live long enough and destroy all sets.
}

void DescriptorSet::UpdateDescriptorSets()
{
    // XXX: Recalculate binding slots?
    // Slots are currently saved in DescriptorItem after creating the layout.
    // u32 bindingIndex = 0;

    std::vector<vk::WriteDescriptorSet> descriptorWrites;

    const size_t bufferDescriptorsCount = desc.layout->desc.bufferDescriptors.size();
    const size_t imageDescriptorsCount = desc.layout->desc.imageDescriptors.size();
    const size_t imageArrayDescriptorsCount = desc.layout->desc.imageArrayDescriptors.size();

    std::vector<vk::DescriptorBufferInfo> bufferDescriptors(bufferDescriptorsCount);
    std::vector<vk::DescriptorImageInfo> imageDescriptors(imageDescriptorsCount);
    std::vector<vk::DescriptorImageInfo> imageArrayDescriptors;

    for (u32 i = 0; i < bufferDescriptorsCount; i++)
    {
        const auto& bd = desc.layout->desc.bufferDescriptors[i];

        bufferDescriptors[i] = vk::DescriptorBufferInfo()
                                   .setBuffer(bd.buffer->buffer)
                                   .setOffset(bd.offset)
                                   .setRange((bd.size > 0) ? bd.size : VK_WHOLE_SIZE);

        descriptorWrites.push_back(MakeVkWriteDescriptorSet(descriptorSet, &bufferDescriptors[i],
                                                            nullptr, bd.descriptorItem.bindingSlot,
                                                            bd.descriptorItem.type));
    }

    for (u32 i = 0; i < imageDescriptorsCount; i++)
    {
        const auto& id = desc.layout->desc.imageDescriptors[i];

        assert(id.image->currentLayout == vk::ImageLayout::eShaderReadOnlyOptimal);

        imageDescriptors[i] = vk::DescriptorImageInfo()
                                  .setSampler(id.image->sampler)
                                  .setImageView(id.image->imageView)
                                  //.setImageLayout(id.image->currentLayout)
                                  .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        descriptorWrites.push_back(MakeVkWriteDescriptorSet(
            descriptorSet, nullptr, &imageDescriptors[i], id.descriptorItem.bindingSlot,
            vk::DescriptorType::eCombinedImageSampler));
    }

    u32 iaOffset = 0;

    for (u32 i = 0; i < imageArrayDescriptorsCount; i++)
    {
        // XXX: Check texture array does not exceed physical device limit.

        const auto& images = desc.layout->desc.imageArrayDescriptors[i].images;
        for (const auto& image : images)
        {
            assert(image->currentLayout == vk::ImageLayout::eShaderReadOnlyOptimal);
            assert((void*)image->sampler != nullptr);
            assert((void*)image->imageView != nullptr);

            const auto imageInfo = vk::DescriptorImageInfo()
                                       .setSampler(image->sampler)
                                       .setImageView(image->imageView)
                                       .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

            imageArrayDescriptors.push_back(imageInfo);
        }

        descriptorWrites.push_back(MakeVkWriteDescriptorSet(
            descriptorSet, nullptr, imageArrayDescriptors.data() + iaOffset,
            desc.layout->desc.imageArrayDescriptors[i].descriptorItem.bindingSlot,
            vk::DescriptorType::eCombinedImageSampler, imageArrayDescriptors.size()));

        iaOffset += static_cast<u32>(images.size());
    }

    m_Context.device.updateDescriptorSets(descriptorWrites, nullptr);
}

} // namespace Vulkan

} // namespace RenderLib
