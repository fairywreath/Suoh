/*
 * Multiple quads + texture array sampler renderer.
 */
#pragma once

#include "RendererBase.h"

namespace Suoh
{

class QuadRenderer : public RendererBase
{
public:
    QuadRenderer(VKRenderDevice* renderDevice, Window* window, const std::vector<std::string>& textureFiles);
    ~QuadRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

    void updateBuffer(u32 i);
    void pushConstants(VkCommandBuffer commandBuffer, u32 textureIndex, const vec2& offset);

    void quad(float x1, float y1, float x2, float y2);
    void clear();

private:
    bool createDescriptors();

    void loadTextureFiles(const std::vector<std::string>& textureFiles);

    struct PushConstantBuffer
    {
        vec2 offset;
        u32 textureIndex;
    };

    struct VertexData
    {
        vec3 pos;
        vec2 tc;
    };

private:
    std::vector<VertexData> mQuads;

    u32 mVertexBufferSize;
    u32 mIndexBufferSize;

    std::vector<Buffer> mStorageBuffers;
    std::vector<Texture> mTextures;
};

} // namespace Suoh
