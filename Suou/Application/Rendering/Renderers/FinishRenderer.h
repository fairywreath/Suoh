#pragma once

#include "RendererBase.h"

namespace Suou
{

class FinishRenderer : public RendererBase
{
public:
    FinishRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);
    ~FinishRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;
};

} // namespace Suou
