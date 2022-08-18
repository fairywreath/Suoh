#pragma once

#include <memory>

#include "RenderLib/Resources/GraphicsPipeline.h"

namespace Suou
{

class VKRenderDevice;

struct IVKPipelineHandlerData
{
};

class VKPipelineHandler
{
public:
    explicit VKPipelineHandler(VKRenderDevice& renderDevice);
    ~VKPipelineHandler();

    GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDescription& desc);
    void destroyGraphicsPipeline(GraphicsPipelineHandle);

private:
    using HandleType = type_safe::underlying_type<GraphicsPipelineHandle>;
    constexpr HandleType toHandleType(GraphicsPipelineHandle handle) const
    {
        return static_cast<HandleType>(handle);
    }

    GraphicsPipelineHandle acquireNewGraphicsHandle();

private:
    VKRenderDevice& mRenderDevice;

    std::unique_ptr<IVKPipelineHandlerData> mData;
};

} // namespace Suou
