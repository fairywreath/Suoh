/*
 * Graphics/rendering constant values.
 */

#pragma once

#include <Core/Types.h>

namespace GPUConstants
{

static constexpr u32 MaxRenderTargets = 8;
static constexpr u32 MaxDescriptorSetLayouts = 8;
static constexpr u32 MaxShaderStages = 5;
static constexpr u32 MaxDescriptorsPerSet = 16;
static constexpr u32 MaxVertexStreams = 16;
static constexpr u32 MaxVertexAttributes = 16;
static constexpr u32 MaxBarriers = 8;
static constexpr u32 MaxFrames = 2;
// static constexpr u32 MaxSwapchainImages = 3;

static constexpr u32 GlobalDescriptorPoolElementSize = 128;
static constexpr u32 CommandBufferDescriptorPoolElementSize = 128;

static constexpr u32 MaxBindlessResources = 1024;

static constexpr u32 SecondaryCommandBuffersCount = 2;

static constexpr u32 DepthStencilClearIndex = MaxRenderTargets;
static constexpr u32 MaxCommandBufferDescriptorSets = 16;

static constexpr u32 MaxShaderBindingIndex = 255;

// Longest graphics pipeline shaders: vertex + tess. conrol + tess. eval + geometry + fragment.
static constexpr u32 MaxShaderStagesPerState = 5;

namespace PoolSize
{

static constexpr u32 Buffers = 16384;
static constexpr u32 Textures = 512;
static constexpr u32 Samplers = 32;
static constexpr u32 RenderPass = 256;
static constexpr u32 DescriptorSetLayouts = 128;
static constexpr u32 DescriptorSets = 4096;
static constexpr u32 Shaders = 128;
static constexpr u32 Pipelines = 128;
static constexpr u32 Framebuffers = 128;

} // namespace PoolSize

} // namespace GPUConstants
