#pragma once

#include "VulkanCommon.h"

#include "VulkanBuffer.h"
#include "VulkanImage.h"

namespace RenderLib
{

namespace Vulkan
{

// XXX: make generic layout type for any kind of buffer/texture attachment

struct DescriptorItem
{
    // XXX: Make this dependent on underlying buffer/image wrapper types?

    vk::DescriptorType type;
    vk::ShaderStageFlags shaderStageFlags;

    // XXX: automatically generated.
    u32 bindingSlot{u32(-1)};
};

struct BufferDescriptorItem
{
    DescriptorItem descriptorItem;

    BufferHandle buffer;
    u32 offset;
    u32 size;
};

struct ImageDesrciptorItem
{
    DescriptorItem descriptorItem;

    ImageHandle image;
};

struct ImageArrayDescriptorItem
{
    DescriptorItem descriptorItem;

    std::vector<ImageHandle> images;
};

struct DescriptorLayoutDesc
{
    std::vector<BufferDescriptorItem> bufferDescriptors;
    std::vector<ImageDesrciptorItem> imageDescriptors;
    std::vector<ImageArrayDescriptorItem> imageArrayDescriptors;

    // Set count for pool.
    u32 poolCountMultiplier;

    // bool automaticSlots{false};
};

class DescriptorLayout : public RefCountResource<IResource>
{
public:
    explicit DescriptorLayout(const VulkanContext& context) : m_Context(context)
    {
    }
    ~DescriptorLayout();

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSetLayout descriptorSetLayout;

    // XXX: Use some kind of descriptor allocator?
    // Right now pool is configured into the layout itself; This pool will be used when creating the
    // actual descriptor sets.
    vk::DescriptorPool descriptorPool;
    std::vector<vk::DescriptorPoolSize> poolSizes;

    DescriptorLayoutDesc desc;

private:
    const VulkanContext& m_Context;
};

using DescriptorLayoutHandle = RefCountPtr<DescriptorLayout>;

struct DescriptorSetDesc
{
    DescriptorSetDesc() = default;
    DescriptorSetDesc(const DescriptorLayoutHandle& _layout) : layout(_layout)
    {
    }

    DescriptorLayoutHandle layout;
};

class DescriptorSet : public RefCountResource<IResource>
{
public:
    explicit DescriptorSet(const VulkanContext& context) : m_Context(context)
    {
    }
    ~DescriptorSet();

    vk::DescriptorSet descriptorSet;

    // This will contain a strong reference to the DescriptorPool object inside the layout handle.
    DescriptorSetDesc desc;

    void UpdateDescriptorSets();

private:
    const VulkanContext& m_Context;
};

using DescriptorSetHandle = RefCountPtr<DescriptorSet>;

} // namespace Vulkan

} // namespace RenderLib
