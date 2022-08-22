#pragma once

#include "RendererBase.h"

namespace Suoh
{

class CubeRenderer : public RendererBase
{
public:
    CubeRenderer(VKRenderDevice* renderDevice, Window* window, const Image& depthImage);
    ~CubeRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

    bool loadCubeMap(const std::string& filePath);

    void updateUniformBuffer(const mat4& mvp);

private:
    bool createDescriptorSets();

    void destroy();

    struct UniformBuffer
    {
        mat4 mvp;
    };

private:
    Texture mTexture;

    bool mIsInitialized{false};
};

} // namespace Suoh