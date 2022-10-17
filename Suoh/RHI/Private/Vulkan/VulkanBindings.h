#pragma once

#include "VulkanCommon.h"

namespace SuohRHI
{

namespace Vulkan
{

// Wraps vk::DescriptorSetLayoutBinding
class VulkanBindingLayout : public RefCountResource<IBindingLayout>
{
public:
    VulkanBindingLayout(const VulkanContext& context, const BindingLayoutDesc& desc);
    VulkanBindingLayout(const VulkanContext& context, const BindlessLayoutDesc& desc);
    ~VulkanBindingLayout() override;

    const BindingLayoutDesc* GetDesc() const override
    {
        return isBindless ? nullptr : &desc;
    }
    const BindlessLayoutDesc* GetBindlessDesc() const override
    {
        return isBindless ? &bindlessDesc : nullptr;
    }
    Object GetNativeObject(ObjectType objectType) override;

    vk::Result GenerateLayout();

    BindingLayoutDesc desc;
    BindlessLayoutDesc bindlessDesc;
    bool isBindless;

    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    std::vector<vk::DescriptorSetLayoutBinding> descriptorLayoutBindings;
    vk::DescriptorSetLayout descriptorSetLayout;

private:
    const VulkanContext& m_Context;
};

// Wraps vk::DescriptorSet
class VulkanBindingSet : public RefCountResource<IBindingSet>
{
public:
    explicit VulkanBindingSet(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanBindingSet() override;

    Object GetNativeObject(ObjectType type) override;
    const BindingSetDesc* GetDesc() const override
    {
        return &desc;
    }
    IBindingLayout* GetLayout() const override
    {
        return layout;
    }

    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    BindingSetDesc desc;
    BindingLayoutHandle layout;

    std::vector<ResourceHandle> resources;

    // XXX: volatile constant buffers?

    std::vector<u32> bindingsInTransitions;

private:
    const VulkanContext& m_Context;
};

class VulkanDescriptorTable : public RefCountResource<IDescriptorTable>
{
public:
    explicit VulkanDescriptorTable(const VulkanContext& context) : m_Context(context)
    {
    }
    ~VulkanDescriptorTable() override;

    Object GetNativeObject(ObjectType type) override;

    const BindingSetDesc* GetDesc() const override
    {
        return nullptr;
    }
    IBindingLayout* GetLayout() const override
    {
        return layout;
    }
    u32 GetCapacity() const override
    {
        return capacity;
    }

    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;

    BindingLayoutHandle layout;
    u32 capacity{0};

private:
    const VulkanContext& m_Context;
};

} // namespace Vulkan

} // namespace SuohRHI