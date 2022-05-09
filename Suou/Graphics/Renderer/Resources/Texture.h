#pragma once

#include <CoreTypes.h>
#include <StrongTypedef.h>

namespace Suou
{
    
STRONG_TYPEDEF(TextureHandle, u32);

struct TextureDescription
{
    const char* path;
};

struct TextureDataDescription
{
    u8* data;
};

}