#pragma once

#include <StrongTypedef.h>
#include "../RenderStates.h"

namespace Suou
{

STRONG_TYPEDEF(BufferHandle, u32);

struct BufferDescription
{
    u64 size{0};
    u32 usage{0};
    BufferMemoryUsage memoryUsage{BufferMemoryUsage::GPU_ONLY};

    // VkBufferUsageFlags
};


}