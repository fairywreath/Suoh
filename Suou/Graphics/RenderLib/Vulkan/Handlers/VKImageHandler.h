#pragma once

#include <vk_mem_alloc.h>

#include "../VKCommon.h"
#include "RenderLib/Resources/Image.h"

namespace Suou
{

struct IVKImageHandlerData
{
};

class VKRenderDevice;

class VKImageHandler
{
public:
    explicit VKImageHandler(VKRenderDevice& renderDevice);
    ~VKImageHandler();

    ImageHandle createImage(const ImageDescription& desc);
    void destroyImage(ImageHandle image);

private:
    using HandleType = type_safe::underlying_type<ImageHandle>;
    inline HandleType toHandleType(ImageHandle handle) const
    {
        return static_cast<HandleType>(handle);
    }

    ImageHandle acquireNewHandle();

private:
    VKRenderDevice& mRenderDevice;

    std::unique_ptr<IVKImageHandlerData> mData;
};

} // namespace Suou
