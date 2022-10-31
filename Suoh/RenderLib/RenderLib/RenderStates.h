#pragma once

#include <CoreTypes.h>

namespace RenderLib
{

enum class BufferUsage : u32
{
    UNKNOWN = 0X00000000,
    STORAGE_BUFFER = 0X00000001,
    VERTEX_BUFFER = 0X00000002,
    INDEX_BUFFER = 0X00000004,
    UNIFORM_BUFFER = 0X00000008,
    INDIRECT_BUFFER = 0X0000010,

    // XXX: Always use internal upload manager?
    // TRANSFER_SOURCE = 0X00000020,
    // TRANSFER_DESTINATION = 0X00000040,
};

enum class CpuAccessMode : u8
{
    NONE,
    READ,
    WRITE,
};

enum class QueueType
{
    GRAPHICS = 0,
    COMPUTE = 1,
    // TRANSFER = 2,
};

} // namespace RenderLib