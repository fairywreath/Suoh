#pragma once

#include "../VKCommon.h"
#include "Renderer/Resources/Buffer.h"

namespace Suou
{

class VKRenderDevice;

struct IVKBufferHandlerData{};

class VKBufferHandler
{
public:
    explicit VKBufferHandler(VKRenderDevice& renderDevice);
    ~VKBufferHandler();

    void destroy();

    BufferHandle createBuffer(const BufferDescription& desc);
    void destroyBuffer(BufferHandle handle);

private:
    using HandleType = type_safe::underlying_type<BufferHandle>;
    inline HandleType toHandleType(BufferHandle handle)
    {
        return static_cast<HandleType>(handle);
    }

    BufferHandle acquireNewHandle();

private:
    VKRenderDevice& mRenderDevice;
    
    std::unique_ptr<IVKBufferHandlerData> mData;

    // friend class VKRenderDevice;
};


    
}