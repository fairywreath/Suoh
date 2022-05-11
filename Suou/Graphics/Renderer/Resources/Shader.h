#pragma once

#include <CoreTypes.h>
#include <StrongTypedef.h>

#include "../RenderStates.h"

namespace Suou
{

STRONG_TYPEDEF(ShaderHandle, u32);

struct ShaderDescription
{
    const char* filePath;

    // do we need this?
    // ShaderType type;
};

} // namespace Suou
