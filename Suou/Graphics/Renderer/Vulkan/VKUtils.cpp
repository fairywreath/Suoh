#include "VKUtils.h"

#include <Asserts.h>

namespace Suou
{

namespace VKUtils
{

VkFormat toVkFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::UNKNOWN:
        assert(false);
        break; // This is an invalid format
    case ImageFormat::R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT; // RGBA32, 128 bits per pixel
    case ImageFormat::R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case ImageFormat::R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case ImageFormat::R32G32B32_FLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT; // RGB32, 96 bits per pixel
    case ImageFormat::R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case ImageFormat::R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case ImageFormat::R16G16B16A16_FLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT; // RGBA16, 64 bits per pixel
    case ImageFormat::R16G16B16A16_UNORM:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case ImageFormat::R16G16B16A16_UINT:
        return VK_FORMAT_R16G16B16A16_UINT;
    case ImageFormat::R16G16B16A16_SNORM:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case ImageFormat::R16G16B16A16_SINT:
        return VK_FORMAT_R16G16B16A16_SINT;
    case ImageFormat::R32G32_FLOAT:
        return VK_FORMAT_R32G32_SFLOAT; // RG32, 64 bits per pixel
    case ImageFormat::R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case ImageFormat::R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case ImageFormat::R10G10B10A2_UNORM:
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32; // RGB10A2, 32 bits per pixel
    case ImageFormat::R10G10B10A2_UINT:
        return VK_FORMAT_A2R10G10B10_UINT_PACK32;
    case ImageFormat::R11G11B10_FLOAT:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32; // RG11B10, 32 bits per pixel
    case ImageFormat::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM; // RGBA8, 32 bits per pixel
    case ImageFormat::R8G8B8A8_UNORM_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case ImageFormat::R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;
    case ImageFormat::R8G8B8A8_SNORM:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case ImageFormat::R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;

    case ImageFormat::B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat::B8G8R8A8_UNORM_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case ImageFormat::B8G8R8A8_SNORM:
        return VK_FORMAT_B8G8R8A8_SNORM;
    case ImageFormat::B8G8R8A8_UINT:
        return VK_FORMAT_B8G8R8A8_UINT;
    case ImageFormat::B8G8R8A8_SINT:
        return VK_FORMAT_B8G8R8A8_SINT;

    case ImageFormat::R16G16_FLOAT:
        return VK_FORMAT_R16G16_SFLOAT; // RG16, 32 bits per pixel
    case ImageFormat::R16G16_UNORM:
        return VK_FORMAT_R16G16_UNORM;
    case ImageFormat::R16G16_UINT:
        return VK_FORMAT_R16G16_UINT;
    case ImageFormat::R16G16_SNORM:
        return VK_FORMAT_R16G16_SNORM;
    case ImageFormat::R16G16_SINT:
        return VK_FORMAT_R16G16_SINT;
    case ImageFormat::R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT; // R32, 32 bits per pixel
    case ImageFormat::R32_UINT:
        return VK_FORMAT_R32_UINT;
    case ImageFormat::R32_SINT:
        return VK_FORMAT_R32_SINT;
    case ImageFormat::R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM; // RG8, 16 bits per pixel
    case ImageFormat::R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case ImageFormat::R8G8_SNORM:
        return VK_FORMAT_R8G8_SNORM;
    case ImageFormat::R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case ImageFormat::R16_FLOAT:
        return VK_FORMAT_R16_SFLOAT; // R16, 16 bits per pixel
    case ImageFormat::D16_UNORM:
        return VK_FORMAT_D16_UNORM; // Depth instead of Red
    case ImageFormat::R16_UNORM:
        return VK_FORMAT_R16_UNORM;
    case ImageFormat::R16_UINT:
        return VK_FORMAT_R16_UINT;
    case ImageFormat::R16_SNORM:
        return VK_FORMAT_R16_SNORM;
    case ImageFormat::R16_SINT:
        return VK_FORMAT_R16_SINT;
    case ImageFormat::R8_UNORM:
        return VK_FORMAT_R8_UNORM; // R8, 8 bits per pixel
    case ImageFormat::R8_UINT:
        return VK_FORMAT_R8_UINT;
    case ImageFormat::R8_SNORM:
        return VK_FORMAT_R8_SNORM;
    case ImageFormat::R8_SINT:
        return VK_FORMAT_R8_SINT;
    default:
        assert(
            "Unknown ImageFormat!"); // We have tried to convert a image format we don't know about, did we just add it?
    }
    return VK_FORMAT_UNDEFINED;
}

VkFormat toVkFormat(DepthImageFormat format)
{
    switch (format)
    {
    case DepthImageFormat::UNKNOWN:
        assert(false);
        break;
    case DepthImageFormat::D32_FLOAT_S8X24_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case DepthImageFormat::D32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case DepthImageFormat::R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case DepthImageFormat::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case DepthImageFormat::D16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case DepthImageFormat::R16_UNORM:
        return VK_FORMAT_R16_UNORM;

    default:
        assert("Unknown DepthImageFormat!");
    }
    return VK_FORMAT_UNDEFINED;
}

VkSampleCountFlagBits toVkSampleCount(SampleCount count)
{
    switch (count)
    {
    case SampleCount::SAMPLE_COUNT_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case SampleCount::SAMPLE_COUNT_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount::SAMPLE_COUNT_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount::SAMPLE_COUNT_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case SampleCount::SAMPLE_COUNT_16:
        return VK_SAMPLE_COUNT_16_BIT;
    default:
        assert("Unknown SampleCount!");
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

VkBufferUsageFlags toVkBufferUsageFlags(u32 usage)
{
    VkBufferUsageFlags flags = 0;
    if (usage & BufferUsage::VERTEX_BUFFER)
    {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::INDEX_BUFFER)
    {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::STORAGE_BUFFER)
    {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (usage & BufferUsage::UNIFORM_BUFFER)
    {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (usage & BufferUsage::INDIRECT_BUFFER)
    {
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (usage & BufferUsage::TRANSFER_SOURCE)
    {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (usage & BufferUsage::TRANSFER_DESTINATION)
    {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    return flags;
}

VkImageUsageFlags toVkImageUsageFlags(u8 usage)
{
    VkImageUsageFlags flags = 0;

    if (usage & ImageUsage::IMAGE_TRANSFER_SOURCE)
    {
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (usage & ImageUsage::IMAGE_TRANSFER_DESTINATION)
    {
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (usage & ImageUsage::SAMPLED)
    {
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (usage & ImageUsage::STORAGE)
    {
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (usage & ImageUsage::COLOR_ATTACHMENT)
    {
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (usage & ImageUsage::DEPTH_STENCIL_ATTACHMENT)
    {
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (usage & ImageUsage::TRANSIENT_ATTACHMENT)
    {
        flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }
    if (usage & ImageUsage::INPUT_ATTACHMENT)
    {
        flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    return flags;
}

VmaMemoryUsage toVmaMemoryUsage(BufferMemoryUsage usage)
{
    VmaMemoryUsage memoryUsage = VmaMemoryUsage::VMA_MEMORY_USAGE_UNKNOWN;

    switch (usage)
    {
    case BufferMemoryUsage::GPU_ONLY:
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        break;
    case BufferMemoryUsage::CPU_ONLY:
        memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
        break;
    case BufferMemoryUsage::CPU_TO_GPU:
        memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        break;
    case BufferMemoryUsage::GPU_TO_CPU:
        memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        break;
    case BufferMemoryUsage::CPU_COPY:
        memoryUsage = VMA_MEMORY_USAGE_CPU_COPY;
        break;
    default:
        assert("Unknown BufferMemoryUsage value!");
        break;
    }

    return memoryUsage;
}

} // namespace VKUtils

} // namespace Suou
