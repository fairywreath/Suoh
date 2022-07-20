#pragma once

#include <string>

#include <CoreTypes.h>
#include <StrongTypedef.h>

#include "../RenderStates.h"

namespace Suou
{

STRONG_TYPEDEF(ShaderHandle, u32);

struct ShaderDescription
{
    std::string filePath;

    // do we need this?
    // ShaderType type;
};

} // namespace Suou
