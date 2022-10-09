#pragma once

#include <Core/Types.h>

#include "../Vulkan/VKCommon.h"

#include <string>
#include <vector>

/*
 * XXX: currently tailored only for Vulkan resources.
 */

namespace Suoh
{

/*
 * Temp. implementation of graphics resource types
 */
struct Buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    u32 size;
};

struct Image
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation;
};

struct Texture
{
    Image image;
    VkSampler sampler;
    VkFormat format;

    u32 width;
    u32 height;
    u32 depth;
};

using ShaderBinary = std::vector<u32>;

struct ShaderModule
{
    ShaderBinary SPIRV;
    size_t size;
    std::string binaryPath;

    VkShaderModule shaderModule;
};

struct GraphicsPipeline
{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

/*
 * Resource descriptors.
 */
struct DescriptorInfo
{
    VkDescriptorType type;
    VkShaderStageFlags shaderStageFlags;
};

struct BufferAttachment
{
    DescriptorInfo descriptorInfo;

    Buffer buffer;
    u32 offset;
    u32 size;
};

// sampler
struct TextureAttachment
{
    DescriptorInfo descriptorInfo;

    Texture texture;
};

struct TextureArrayAttachment
{
    DescriptorInfo descriptorInfo;

    std::vector<Texture> textures;
};

// texture creation params
struct TextureDescription
{
    VkFilter minFilter;
    VkFilter maxFilter;

    ///	VkAccessModeFlags accessMode;
};

struct DescriptorSetInfo
{
    std::vector<BufferAttachment> buffers;
    std::vector<TextureAttachment> textures;
    std::vector<TextureArrayAttachment> textureArrays;
};

struct GraphicsPipelineInfo
{
    u32 width{0};
    u32 height{0};

    VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

    bool useDepth{true};

    bool useBlending{true};

    bool dynamicScissorState{false};

    u32 patchControlPoints{0};
};

} // namespace Suoh