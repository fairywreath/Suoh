#pragma once

#include <SuouBase.h>

#include "Resources/Buffer.h"

namespace Suou
{
    
class RenderDevice
{
public:
    virtual ~RenderDevice() {};

    virtual void destroy() = 0;

protected:
    RenderDevice() {};
    
};

}