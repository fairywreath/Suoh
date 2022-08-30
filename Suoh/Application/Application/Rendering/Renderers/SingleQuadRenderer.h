#pragma once

#include "RendererBase.h"

namespace Suoh
{

class SingleQuadRenderer : public RendererBase
{
public:
    SingleQuadRenderer(VKRenderDevice* renderDevice, Window* window);
    ~SingleQuadRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

    bool init(const Texture& texture, VkImageLayout desiredLayout = VK_IMAGE_LAYOUT_GENERAL);

private:
    bool createDescriptors(VkImageLayout desiredLayout);

private:
    Texture mTexture;
};

} // namespace Suoh
