#pragma once

#include "VulkanCommon.h"

struct StencilOperationState
{
    vk::StencilOp pass{vk::StencilOp::eKeep};
    vk::StencilOp fail{vk::StencilOp::eKeep};
    vk::StencilOp depthFail{vk::StencilOp::eKeep};

    vk::CompareOp compare{vk::CompareOp::eAlways};

    u32 compareMask{0};
    u32 writeMask{0};
    u32 reference{0};
};

struct DepthStencilState
{
    bool depthEnable{true};
    bool depthWriteEnable{true};
    vk::CompareOp depthCompare{vk::CompareOp::eAlways};

    // XXX: Stencil state not yet supported.
    bool stencilEnable{false};
    StencilOperationState stencilFront;
    StencilOperationState stencilBack;

    DepthStencilState& SetDepth(bool enable, bool write, vk::CompareOp compareOp)
    {
        depthEnable = true;
        depthWriteEnable = write;
        depthCompare = compareOp;
        return *this;
    }
    DepthStencilState& SetStencil(bool enable, const StencilOperationState& frontState,
                                  const StencilOperationState& backState)
    {
        stencilEnable = enable;
        stencilFront = frontState;
        stencilBack = backState;
        return *this;
    }
};

struct BlendState
{
    vk::BlendFactor sourceColor{vk::BlendFactor::eOne};
    vk::BlendFactor destinationColor{vk::BlendFactor::eOne};
    vk::BlendOp colorOperation{vk::BlendOp::eAdd};

    vk::BlendFactor sourceAlpha{vk::BlendFactor::eOne};
    vk::BlendFactor destinationAlpha{vk::BlendFactor::eOne};
    vk::BlendOp alphaOperation{vk::BlendOp::eAdd};

    ColorWriteEnableFlags colorWriteFlags{ColorWriteEnableFlags::All};

    bool blendEnable{false};

    // If set to false, alpha blend parameters will use color parameters.
    bool blendSeparate{false};

    BlendState& SetColor(vk::BlendFactor source, vk::BlendFactor destination, vk::BlendOp operation)
    {
        sourceColor = source;
        destinationColor = destination;
        colorOperation = operation;
        return *this;
    }
    BlendState& SetAlpha(vk::BlendFactor source, vk::BlendFactor destination, vk::BlendOp operation)
    {
        sourceAlpha = source;
        destinationAlpha = destination;
        alphaOperation = operation;
        return *this;
    }
    BlendState& SetBlendEnable(bool value)
    {
        blendEnable = value;
        return *this;
    }
    BlendState& SetBlendSeparate(bool value)
    {
        blendSeparate = value;
        return *this;
    }
    BlendState& SetColorWrite(ColorWriteEnableFlags flags)
    {
        colorWriteFlags = flags;
        return *this;
    }
};

struct BlendStatesDesc
{
    std::vector<BlendState> blendStates;

    BlendStatesDesc& Reset()
    {
        blendStates.clear();
    }
    BlendStatesDesc& AddBlendState(const BlendState& state)
    {
        blendStates.push_back(state);
    }
};

struct RasterizationState
{
    vk::CullModeFlags cullMode{vk::CullModeFlagBits::eNone};
    vk::FrontFace frontFace{vk::FrontFace::eCounterClockwise};
    vk::PolygonMode fillMode{vk::PolygonMode::eFill};

    RasterizationState& SetCullMode(vk::CullModeFlagBits flagBits)
    {
        cullMode = flagBits;
        return *this;
    }
    RasterizationState& SetFrontFace(vk::FrontFace face)
    {
        frontFace = face;
        return *this;
    }
    RasterizationState& SetFillMode(vk::PolygonMode mode)
    {
        fillMode = mode;
        return *this;
    }
};

struct VertexAttribute
{
    u32 location{0};
    u32 binding{0};
    u32 offset{0};
    // VertexComponentFormat format{VertexComponentFormat::Count};
    vk::Format format;
};

struct VertexStream
{
    u32 binding{0};
    u32 stride{0};
    VertexInputRate inputRate{VertexInputRate::Count};
};

struct VertexInputDesc
{
    std::vector<VertexStream> vertexStreams;
    std::vector<VertexAttribute> vertexAttributes;

    VertexInputDesc& Reset()
    {
        vertexStreams.clear();
        vertexAttributes.clear();
        return *this;
    }
    VertexInputDesc& AddVertexStream(const VertexStream& stream)
    {
        vertexStreams.push_back(stream);
        return *this;
    }
    VertexInputDesc& AddVertexAttribute(const VertexAttribute& attribute)
    {
        vertexAttributes.push_back(attribute);
        return *this;
    }
};

struct ResourceUpdate
{
    ResourceType type;
    GPUResource* resource;

    u32 currentFrame{0};
    bool deleting{false};

    ResourceUpdate& SetType(ResourceType _type)
    {
        type = _type;
        return *this;
    }
    ResourceUpdate& SetResource(GPUResource* _resource)
    {
        resource = _resource;
        return *this;
    }
};

struct DescriptorSetUpdate
{
    // DescriptorSet* descriptorSet;
    // u32 frameIssued{0};
};

struct ComputeLocalSize
{
    u32 x : 10;
    u32 y : 10;
    u32 z : 10;
    u32 pad : 2;
};

class GPUTimeQueryTree;

struct GPUThreadFramePools
{
    vk::CommandPool commandPool;
    vk::QueryPool timestampQueryPool;
    vk::QueryPool pipelineStatsQuerypool;

    GPUTimeQueryTree* timeQueries{nullptr};
};
