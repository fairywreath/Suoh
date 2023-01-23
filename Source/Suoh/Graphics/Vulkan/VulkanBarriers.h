#pragma once

#include "VulkanResources.h"

struct ImageBarrier
{
    Texture* texture;
};

struct MemoryBarrier
{
    Buffer* buffer;
};

struct ExecutionBarrier
{
    PipelineStage sourcePipelinStage;
    PipelineStage destinationPipelineStage;

    u32 newBarrierExperimental{std::numeric_limits<u32>::max()};
    u32 loadOperation{0};

    std::vector<ImageBarrier> imageBarriers;
    std::vector<MemoryBarrier> memoryBarriers;

    ExecutionBarrier& Reset();
    ExecutionBarrier& SetStages(PipelineStage source, PipelineStage destination);
    ExecutionBarrier& AddImageBarrier(const ImageBarrier& imageBarrier);
    ExecutionBarrier& AddMemoryBarrier(const MemoryBarrier& memoryBarrier);
};
