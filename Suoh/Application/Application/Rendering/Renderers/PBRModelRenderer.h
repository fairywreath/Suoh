#pragma once

#include "RendererBase.h"

namespace Suoh
{

class PBRModelRenderer : public RendererBase
{
public:
    PBRModelRenderer(VKRenderDevice* renderDevice,
                     Window* window,
                     Image depthImage,
                     u32 uniformBufferSize,
                     const std::string& modelFile,
                     const std::string& texAOFile,
                     const std::string& texEmissiveFile,
                     const std::string& texAlbedoFile,
                     const std::string& texMeRFile,
                     const std::string& texNormalFile,
                     const std::string& texEnvMapFile,
                     const std::string& texIrrMapFile);
    ~PBRModelRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;
    void updateUniformBuffer(const void* data, size_t dataSize);

    struct UniformBuffer
    {
        mat4 mvp, mv, m;
        vec4 cameraPos;
    };

private:
    bool createDescriptors(u32 uniformDataSize);

private:
    size_t mVertexBufferSize;
    size_t mIndexBufferSize;
    size_t mIndexBufferOffset;

    Buffer mStorageBuffer;

    Texture mTextureAO;
    Texture mTextureEmissive;
    Texture mTextureAlbedo;
    Texture mTextureMeR;
    Texture mTextureNormal;

    Texture mEnvMapIrradiance;
    Texture mEnvMap;

    Texture mBrdfLUT;
};

} // namespace Suoh
