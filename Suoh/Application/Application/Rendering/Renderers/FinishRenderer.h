#pragma once

#include "RendererBase.h"

namespace Suoh
{

class FinishRenderer : public RendererBase
{
public:
    FinishRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);
    ~FinishRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;
};

} // namespace Suoh
