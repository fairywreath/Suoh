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

    virtual BufferHandle createBuffer(const BufferDescription& desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;

protected:
    RenderDevice() {};
    
};

}