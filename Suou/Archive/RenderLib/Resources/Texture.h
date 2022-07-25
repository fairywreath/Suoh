#pragma once

#include <CoreTypes.h>
#include <StrongTypedef.h>

namespace Suou
{

STRONG_TYPEDEF(TextureHandle, u32);

struct TextureDescription
{
    std::string path;
};

struct TextureDataDescription
{
    u8* data;
};

} // namespace Suou
