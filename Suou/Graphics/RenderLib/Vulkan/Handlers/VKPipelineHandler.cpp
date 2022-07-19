#include "VKPipelineHandler.h"
#include "../VKRenderDevice.h"

#include <queue>

namespace Suou
{

struct GraphicsPipeline
{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

struct VKPipelineHandlerData : IVKPipelineHandlerData
{
    std::vector<GraphicsPipeline> graphicsPipelines;
    std::queue<GraphicsPipelineHandle> returnedGraphicsPipelineHandles;
};

VKPipelineHandler::VKPipelineHandler(VKRenderDevice& renderDevice)
    : mRenderDevice(renderDevice),
      mData(std::make_unique<VKPipelineHandlerData>())
{
}

static constexpr VKPipelineHandlerData& toPipelineHandlerData(IVKPipelineHandlerData* data)
{
    return static_cast<VKPipelineHandlerData&>(*data);
}

VKPipelineHandler::~VKPipelineHandler()
{
}

GraphicsPipelineHandle VKPipelineHandler::createGraphicsPipeline(const GraphicsPipelineDescription& desc)
{
    return GraphicsPipelineHandle::Invalid();
}

void VKPipelineHandler::destroyGraphicsPipeline(GraphicsPipelineHandle)
{
}

GraphicsPipelineHandle VKPipelineHandler::acquireNewGraphicsHandle()
{
    GraphicsPipelineHandle handle;
    auto& data = toPipelineHandlerData(mData.get());

    if (!data.returnedGraphicsPipelineHandles.empty())
    {
        handle = data.returnedGraphicsPipelineHandles.front();
        data.returnedGraphicsPipelineHandles.pop();
    }
    else
    {
        handle = GraphicsPipelineHandle(static_cast<HandleType>(data.graphicsPipelines.size()));
        data.graphicsPipelines.emplace_back();
    }

    return handle;
}

} // namespace Suou
