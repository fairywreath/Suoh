#pragma once

namespace SuohRHI
{

constexpr u64 C_VERSION_SUBMITTED_FLAG = 0x8000000000000000;
constexpr u32 C_VERSION_QUEUE_SHIFT = 60;
constexpr u32 C_VERSION_QUEUE_MASK = 0x7;
constexpr u64 C_VERSION_ID_MASK = 0x0FFFFFFFFFFFFFFF;

constexpr u64 MakeVersion(u64 id, CommandQueue queue, bool submitted)
{
    u64 result = (id & C_VERSION_ID_MASK) | (u64(queue) << C_VERSION_QUEUE_SHIFT);
    if (submitted)
        result |= C_VERSION_SUBMITTED_FLAG;
    return result;
}

constexpr u64 VersionGetInstance(u64 version)
{
    return version & C_VERSION_ID_MASK;
}

constexpr CommandQueue VersionGetQueue(u64 version)
{
    return CommandQueue((version >> C_VERSION_QUEUE_SHIFT) & C_VERSION_QUEUE_MASK);
}

constexpr bool VersionGetSubmitted(u64 version)
{
    return (version & C_VERSION_SUBMITTED_FLAG) != 0;
}

} // namespace SuohRHI
