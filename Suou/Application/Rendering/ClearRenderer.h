#pragma once

#include "RendererBase.h"

namespace Suou
{

class ClearRenderer : public RendererBase
{
public:
    ClearRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);
    ~ClearRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

private:
    bool mClearDepth;
};

} // namespace Suou
