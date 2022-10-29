#include "VulkanUtils.h"

namespace RenderLib
{

namespace Vulkan
{

vk::BufferUsageFlags ToVkBufferUsageFlags(BufferUsage usage)
{
    vk::BufferUsageFlags ret(0);

    if (usage == BufferUsage::STORAGE_BUFFER)
        ret = vk::BufferUsageFlagBits::eStorageBuffer;
    if (usage == BufferUsage::VERTEX_BUFFER)
        ret = vk::BufferUsageFlagBits::eVertexBuffer;
    if (usage == BufferUsage::INDEX_BUFFER)
        ret = vk::BufferUsageFlagBits::eIndexBuffer;
    if (usage == BufferUsage::UNIFORM_BUFFER)
        ret = vk::BufferUsageFlagBits::eUniformBuffer;
    if (usage == BufferUsage::INDIRECT_BUFFER)
        ret = vk::BufferUsageFlagBits::eIndirectBuffer;

    return ret;
}

u32 BytesPerTexFormat(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eR8Sint:
    case vk::Format::eR8Unorm:
        return 1;
    case vk::Format::eR16Sfloat:
        return 2;
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eR8G8B8A8Unorm:
        return 4;
    case vk::Format::eR16G16B16A16Sfloat:
        return 4 * sizeof(u16);
    case vk::Format::eR32G32B32A32Sfloat:
        return 4 * sizeof(float);
    default:
        break;
    }
    return 0;
}

bool HasStencilComponent(vk::Format format)
{
    if ((format == vk::Format::eD32SfloatS8Uint) || (format == vk::Format::eD24UnormS8Uint))
    {
        return true;
    }

    return false;
}

static glslang_stage_t ToGlslangShaderStageFromFileName(const std::string& fileName)
{
    if (fileName.ends_with(".vert"))
        return GLSLANG_STAGE_VERTEX;

    if (fileName.ends_with(".frag"))
        return GLSLANG_STAGE_FRAGMENT;

    if (fileName.ends_with(".geom"))
        return GLSLANG_STAGE_GEOMETRY;

    if (fileName.ends_with(".comp"))
        return GLSLANG_STAGE_COMPUTE;

    if (fileName.ends_with(".tesc"))
        return GLSLANG_STAGE_TESSCONTROL;

    if (fileName.ends_with(".tese"))
        return GLSLANG_STAGE_TESSEVALUATION;

    return GLSLANG_STAGE_VERTEX;
}

} // namespace Vulkan

} // namespace RenderLib
