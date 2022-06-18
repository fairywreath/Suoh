#pragma once

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
    VKRenderDevice& mRenderDevice;
};

} // namespace Suou