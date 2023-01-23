#include "VulkanResources.h"

#include <Core/Logger.h>

vk::Format ToVkFormatVertex(VertexComponentFormat value)
{
    // Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2,
    // Uint4, Count
    // static VkFormat s_vk_vertex_formats[VertexComponentFormat::Count] = {VK_FORMAT_R32_SFLOAT,
    //                                                                      VK_FORMAT_R32G32_SFLOAT,
    //                                                                      VK_FORMAT_R32G32B32_SFLOAT,
    //                                                                      VK_FORMAT_R32G32B32A32_SFLOAT,
    //                                                                      /*MAT4 TODO*/ VK_FORMAT_R32G32B32A32_SFLOAT,
    //                                                                      VK_FORMAT_R8_SINT,
    //                                                                      VK_FORMAT_R8G8B8A8_SNORM,
    //                                                                      VK_FORMAT_R8_UINT,
    //                                                                      VK_FORMAT_R8G8B8A8_UINT,
    //                                                                      VK_FORMAT_R16G16_SINT,
    //                                                                      VK_FORMAT_R16G16_SNORM,
    //                                                                      VK_FORMAT_R16G16B16A16_SINT,
    //                                                                      VK_FORMAT_R16G16B16A16_SNORM,
    //                                                                      VK_FORMAT_R32_UINT,
    //                                                                      VK_FORMAT_R32G32_UINT,
    //                                                                      VK_FORMAT_R32G32B32A32_UINT};

    // return s_vk_vertex_formats[value];

    vk::Format res = vk::Format::eUndefined;

    switch (value)
    {
    case VertexComponentFormat::Float:
        res = vk::Format::eR32Sfloat;
        break;
    case VertexComponentFormat::Float2:
        res = vk::Format::eR32G32Sfloat;
        break;
    case VertexComponentFormat::Float3:
        res = vk::Format::eR32G32B32Sfloat;
        break;
    case VertexComponentFormat::Float4:
        res = vk::Format::eR32G32B32A32Sfloat;
        break;
    case VertexComponentFormat::Uint:
        res = vk::Format::eR32Uint;
        break;
    case VertexComponentFormat::Uint2:
        res = vk::Format::eR32G32Uint;
        break;
    default:
        res = vk::Format::eUndefined;
        break;
    }

    return res;
}

vk::ImageType ToVkImageType(TextureType type)
{
    vk::ImageType res;

    switch (type)
    {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        res = vk::ImageType::e1D;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
        res = vk::ImageType::e2D;
        break;
    case TextureType::Texture3D:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        res = vk::ImageType::e3D;
        break;
    default:
        res = vk::ImageType::e2D;
        break;
    }

    return res;
}

vk::ImageViewType ToVkImageViewType(TextureType type)
{
    vk::ImageViewType res;

    switch (type)
    {
    case TextureType::Texture1D:
        res = vk::ImageViewType::e1D;
        break;
    case TextureType::Texture1DArray:
        res = vk::ImageViewType::e1DArray;
        break;
    case TextureType::Texture2D:
        res = vk::ImageViewType::e2D;
        break;
    case TextureType::Texture2DArray:
        res = vk::ImageViewType::e2DArray;
        break;
    case TextureType::Texture3D:
        res = vk::ImageViewType::e3D;
        break;
    case TextureType::TextureCube:
        res = vk::ImageViewType::eCube;
        ;
        break;
    case TextureType::TextureCubeArray:
        res = vk::ImageViewType::eCubeArray;
        break;
    default:
        LOG_ERROR("Invalid TextureType enum!");
        res = vk::ImageViewType::e2D;
        break;
    }

    return res;
}

vk::PipelineStageFlags ToVkPipelineStageFlags(PipelineStage stage)
{
    return vk::PipelineStageFlags(0);
}

vk::PrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology value)
{
    vk::PrimitiveTopology res;

    switch (value)
    {
    case PrimitiveTopology::Point:
        res = vk::PrimitiveTopology::ePointList;
        break;
    case PrimitiveTopology::Line:
        res = vk::PrimitiveTopology::eLineList;
        break;
    case PrimitiveTopology::Triangle:
        res = vk::PrimitiveTopology::eTriangleList;
        break;
    case PrimitiveTopology::Patch:
        res = vk::PrimitiveTopology::ePatchList;
        break;
    default:
        res = vk::PrimitiveTopology::eTriangleList;
        break;
    }

    return res;
}

std::string ToShaderStageDefine(vk::ShaderStageFlagBits flagBits)
{
    switch (flagBits)
    {
    case vk::ShaderStageFlagBits::eVertex:
        return "VERTEX";
    case vk::ShaderStageFlagBits::eFragment:
        return "FRAGMENT";
    case vk::ShaderStageFlagBits::eCompute:
        return "COMPUTE";
    case vk::ShaderStageFlagBits::eMeshNV:
        return "MESH";
    case vk::ShaderStageFlagBits::eTaskNV:
        return "TASK";
    default:
        return "";
    }

    return "";
}

std::string ToShaderCompilerExtension(vk::ShaderStageFlagBits flagBits)
{
    switch (flagBits)
    {
    case vk::ShaderStageFlagBits::eVertex:
        return "vert";
    case vk::ShaderStageFlagBits::eFragment:
        return "frag";
    case vk::ShaderStageFlagBits::eCompute:
        return "comp";
    case vk::ShaderStageFlagBits::eMeshNV:
        return "mesh";
    case vk::ShaderStageFlagBits::eTaskNV:
        return "task";
    default:
        return "";
    }

    return "";
}

RenderPassOutput& RenderPassOutput::Reset()
{
    colorTargets.clear();

    depthStencilFormat = vk::Format::eUndefined;
    depthStencilFinalLayout = vk::ImageLayout::eUndefined;
    depthOperation = RenderPassOperation::DontCare;
    stencilOperation = RenderPassOperation::DontCare;

    return *this;
}

RenderPassOutput& RenderPassOutput::AddColorTarget(vk::Format format, vk::ImageLayout layout,
                                                   RenderPassOperation loadOp)
{
    colorTargets.push_back(RenderPassColorTarget(format, layout, loadOp));

    return *this;
}

RenderPassOutput& RenderPassOutput::SetDepthStencilTarget(vk::Format format, vk::ImageLayout layout)
{
    depthStencilFormat = format;
    depthStencilFinalLayout = layout;

    return *this;
}
RenderPassOutput& RenderPassOutput::SetDepthStencilOperations(RenderPassOperation _depthOp,
                                                              RenderPassOperation _stencilOp)
{
    depthOperation = _depthOp;
    stencilOperation = _stencilOp;

    return *this;
}

RenderPassOutput& RenderPassOutput::SetName(const std::string& _name)
{
    name = _name;

    return *this;
}