#pragma once

#include <CoreTypes.h>

namespace RenderLib
{

// XXX: use underlying_type instead of hardcoding u32
#define ENUM_CLASS_FLAG_OPERATORS(T)                                                               \
    inline T operator|(T a, T b)                                                                   \
    {                                                                                              \
        return T(u32(a) | u32(b));                                                                 \
    }                                                                                              \
    inline T operator&(T a, T b)                                                                   \
    {                                                                                              \
        return T(u32(a) & u32(b));                                                                 \
    }                                                                                              \
    inline T operator~(T a)                                                                        \
    {                                                                                              \
        return T(~u32(a));                                                                         \
    }                                                                                              \
    inline bool operator!(T a)                                                                     \
    {                                                                                              \
        return u32(a) == 0;                                                                        \
    }                                                                                              \
    inline bool operator==(T a, u32 b)                                                             \
    {                                                                                              \
        return u32(a) == b;                                                                        \
    }                                                                                              \
    inline bool operator!=(T a, u32 b)                                                             \
    {                                                                                              \
        return u32(a) != b;                                                                        \
    }

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