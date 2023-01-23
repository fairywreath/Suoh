#pragma once

#include <span>

#include "VulkanResources.h"

class VulkanDevice;

struct GPUTimeQuery
{
    f64 elapsedMs;

    u32 startQueryIndex;
    u32 endQueryIndex;

    u32 parentIndex;
    u32 depth;

    u32 color;
    u32 frameIndex;

    std::string name;
};

struct GPUTimeQueryTree
{
    void Reset();
    void SetQueries(std::span<GPUTimeQuery> timeQueries);

    GPUTimeQuery Push(const std::string& name);
    GPUTimeQuery Pop();

    // XXX: Non owning view?
    std::span<GPUTimeQuery> timeQueries;

    u32 currentTimeQuery{0};
    bool allocatedTimeQuery{false};
    u32 depth{0};
};

struct GPUPipelineStatistics
{
    enum Statistics : u8
    {
        VerticesCount,
        PrimitiveCount,
        VertexShaderInvocations,
        ClippingInvocations,
        ClippingPrimitives,
        FragmentShaderInvocations,
        ComputeShaderInvocations,
        Count
    };

    void Reset();

    u64 Statistics[Count];
};

class GPUTimeQueriesManager
{
public:
    void Init(std::span<GPUThreadFramePools> threadFramePools, u32 queriesPerThread, u32 numThreads,
              u32 maxFrames);

    void Reset();
    u32 Resolve(u32 currentFrame, GPUTimeQuery* timestampsToFill);

private:
    std::vector<GPUTimeQueryTree> m_QueryTrees;
    std::vector<GPUThreadFramePools> m_ThreadFramePools;
    std::vector<GPUTimeQuery> m_TimeStamps;

    GPUPipelineStatistics m_FramePipelineStatistics;

    u32 m_QueriesPerFrame{0};
    u32 m_QueriesPerThread{0};
    u32 m_NumThreads{0};
    bool m_CurrentFrameResolved{false};
};

class GPVisualProfiler
{
public:
    void Init(u32 maxFrames, u32 maxQueriesPerFrame);

    void Update(VulkanDevice& device);

    void DrawImgui();

private:
    GPUTimeQuery* timestamps;
    u32* perFrameActive;
    GPUPipelineStatistics* pipelineStatistics;

    u32 maxFrames;
    u32 maxQueriesPerFrame;
    u32 currentFrame;

    f32 maxTime;
    f32 minTime;
    f32 averageTime;

    f32 maxDuration;
    bool paused;
};