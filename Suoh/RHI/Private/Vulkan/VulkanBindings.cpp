#include "VulkanBindings.h"

#include "VulkanUtils.h"

namespace SuohRHI
{

namespace Vulkan
{

VulkanBindingLayout::VulkanBindingLayout(const VulkanContext& context, const BindingLayoutDesc& desc_)
    : m_Context(context), desc(desc_), isBindless(false)
{
    auto shaderStageFlags = ToVkShaderStageFlagBits(desc.visibility);

    for (const auto& binding : desc.bindings)
    {
        vk::DescriptorType descriptorType;
        u32 descriptorCount = 1;

        // hlsl -> spirv register allocation?
        // u32 registerOffset = 0;

        switch (binding.type)
        {
        case BindingResourceType::SAMPLER:
            descriptorType = vk::DescriptorType::eSampler;
            break;
        case BindingResourceType::STORAGE_IMAGE:
            descriptorType = vk::DescriptorType::eStorageImage;
            break;
        case BindingResourceType::UNIFORM_TEXEL_BUFFER:
            descriptorType = vk::DescriptorType::eUniformTexelBuffer;
            break;
        case BindingResourceType::STORAGE_BUFFER:
            descriptorType = vk::DescriptorType::eStorageBuffer;
            break;
        case BindingResourceType::STORAGE_TEXEL_BUFFER:
            descriptorType = vk::DescriptorType::eStorageTexelBuffer;
            break;
        case BindingResourceType::UNIFORM_BUFFER:
            descriptorType = vk::DescriptorType::eUniformBuffer;
            break;
        case BindingResourceType::UNIFORM_BUFFER_DYNAMIC:
            descriptorType = vk::DescriptorType::eUniformBufferDynamic;
            break;
        case BindingResourceType::PUSH_CONSTANTS:
            descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorCount = 0;
            break;
        default:
            LOG_ERROR("VulkanBindingLayout: Invalid BindingResourceType enum!");
            continue;
        }

        // Handle registerOffset for non glsl compilations?
        const auto bindingLocation = binding.slot;

        const auto vkBinding = vk::DescriptorSetLayoutBinding()
                                   .setBinding(bindingLocation)
                                   .setDescriptorCount(descriptorCount)
                                   .setDescriptorType(descriptorType)
                                   .setStageFlags(shaderStageFlags);

        descriptorLayoutBindings.push_back(vkBinding);
    }
}

VulkanBindingLayout::VulkanBindingLayout(const VulkanContext& context, const BindlessLayoutDesc& desc_)
    : m_Context(context), bindlessDesc(desc_), isBindless(true)

{
    desc.visibility = bindlessDesc.visibility;

    auto shaderStageFlags = ToVkShaderStageFlagBits(bindlessDesc.visibility);
    u32 bindingLocation = 0;
    u32 arraySize = bindlessDesc.maxCapacity;

    for (const auto& space : bindlessDesc.registerSpaces)
    {
        vk::DescriptorType descriptorType;

        switch (space.type)
        {
        case BindingResourceType::SAMPLER:
            descriptorType = vk::DescriptorType::eSampler;
            break;
        case BindingResourceType::STORAGE_IMAGE:
            descriptorType = vk::DescriptorType::eStorageImage;
            break;
        case BindingResourceType::UNIFORM_TEXEL_BUFFER:
            descriptorType = vk::DescriptorType::eUniformTexelBuffer;
            break;
        case BindingResourceType::STORAGE_BUFFER:
            descriptorType = vk::DescriptorType::eStorageBuffer;
            break;
        case BindingResourceType::STORAGE_TEXEL_BUFFER:
            descriptorType = vk::DescriptorType::eStorageTexelBuffer;
            break;
        case BindingResourceType::UNIFORM_BUFFER:
            descriptorType = vk::DescriptorType::eUniformBuffer;
            break;
        case BindingResourceType::UNIFORM_BUFFER_DYNAMIC:
            descriptorType = vk::DescriptorType::eUniformBufferDynamic;
            break;
        case BindingResourceType::PUSH_CONSTANTS:
            continue;
            break;
        default:
            LOG_ERROR("VulkanBindingLayout: Invalid BindingResourceType enum!");
            continue;
        }

        const auto vkBinding = vk::DescriptorSetLayoutBinding()
                                   .setBinding(bindingLocation)
                                   .setDescriptorCount(arraySize)
                                   .setDescriptorType(descriptorType)
                                   .setStageFlags(shaderStageFlags);

        descriptorLayoutBindings.push_back(vkBinding);

        bindingLocation++;
    }
}

VulkanBindingLayout::~VulkanBindingLayout()
{
    if (descriptorSetLayout)
    {
        m_Context.device.destroyDescriptorSetLayout(descriptorSetLayout, nullptr);
        descriptorSetLayout = vk::DescriptorSetLayout();
    }
}

Object VulkanBindingLayout::GetNativeObject(ObjectType objectType)
{
    switch (objectType)
    {
    case ObjectTypes::Vk_DescriptorSetLayout:
        return Object(descriptorSetLayout);
    default:
        return Object(nullptr);
    }
}

vk::Result VulkanBindingLayout::GenerateLayout()
{
    const u32 bindingsCount = descriptorLayoutBindings.size();

    auto descriptorSetLayoutInfo
        = vk::DescriptorSetLayoutCreateInfo().setBindingCount(bindingsCount).setPBindings(descriptorLayoutBindings.data());

    // Setup for bindless.
    std::vector<vk::DescriptorBindingFlags> bindingFlags(bindingsCount, vk::DescriptorBindingFlagBits::ePartiallyBound);
    auto extendedInfo
        = vk::DescriptorSetLayoutBindingFlagsCreateInfo().setBindingCount(bindingsCount).setPBindingFlags(bindingFlags.data());
    if (isBindless)
    {
        descriptorSetLayoutInfo.setPNext(&extendedInfo);
    }

    auto res = m_Context.device.createDescriptorSetLayout(&descriptorSetLayoutInfo, nullptr, &descriptorSetLayout);
    VK_CHECK_RETURN_RESULT(res);

    // Count number of descriptors per type and descriptor pool sizes.
    std::unordered_map<vk::DescriptorType, u32> poolSizeMap;

    for (const auto& layoutBinding : descriptorLayoutBindings)
    {
        if (!poolSizeMap.contains(layoutBinding.descriptorType))
            poolSizeMap[layoutBinding.descriptorType] = 0;

        poolSizeMap[layoutBinding.descriptorType] += layoutBinding.descriptorCount;
    }

    for (auto iter : poolSizeMap)
    {
        if (iter.second > 0)
        {
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize().setType(iter.first).setDescriptorCount(iter.second));
        }
    }

    return vk::Result::eSuccess;
}

VulkanBindingSet::~VulkanBindingSet()
{
    if (descriptorPool)
    {
        m_Context.device.destroyDescriptorPool(descriptorPool);
        descriptorPool = vk::DescriptorPool();
        descriptorSet = vk::DescriptorSet();
    }
}

Object VulkanBindingSet::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_DescriptorPool:
        return Object(descriptorPool);
    case ObjectTypes::Vk_DescriptorSet:
        return Object(descriptorSet);
    default:
        return Object(nullptr);
    }
}

VulkanDescriptorTable::~VulkanDescriptorTable()
{
    if (descriptorPool)
    {
        m_Context.device.destroyDescriptorPool(descriptorPool);
        descriptorPool = vk::DescriptorPool();
        descriptorSet = vk::DescriptorSet();
    }
}

Object VulkanDescriptorTable::GetNativeObject(ObjectType type)
{
    switch (type)
    {
    case ObjectTypes::Vk_DescriptorPool:
        return Object(descriptorPool);
    case ObjectTypes::Vk_DescriptorSet:
        return Object(descriptorSet);
    default:
        return Object(nullptr);
    }
}

} // namespace Vulkan

} // namespace SuohRHI