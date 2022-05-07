#pragma once

#include <CoreTypes.h>
#include <StrongTypedef.h>

namespace Suou
{

STRONG_TYPEDEF(BufferHandle, u32);

struct BufferDescription
{
    u64 size = 0;

    // TODO
    // VmaMemoryUsage
    // VkBufferUsageFlags
};


}