#pragma once

#include "../VKCommon.h"
#include "Renderer/Resources/Buffer.h"

namespace Suou
{

struct IVKBufferHandlerData
{
};

class VKRenderDevice;

class VKBufferHandler
{
public:
    explicit VKBufferHandler(VKRenderDevice& renderDevice);
    ~VKBufferHandler();

    void destroy();

    BufferHandle createBuffer(const BufferDescription& desc);
    void destroyBuffer(BufferHandle handle);

    VkBuffer getVkBuffer(BufferHandle handle) const;

private:
    using HandleType = type_safe::underlying_type<BufferHandle>;
    inline HandleType toHandleType(BufferHandle handle) const
    {
        return static_cast<HandleType>(handle);
    }

    BufferHandle acquireNewHandle();

private:
    VKRenderDevice& mRenderDevice;

    std::unique_ptr<IVKBufferHandlerData> mData;
};

} // namespace Suou
