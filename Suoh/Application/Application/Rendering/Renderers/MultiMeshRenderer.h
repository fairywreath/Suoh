#pragma once

#include "RendererBase.h"

#include <Graphics/Scene/VertexData.h>

namespace Suoh
{

class MultiMeshRenderer : public RendererBase
{
public:
    MultiMeshRenderer(VKRenderDevice* renderDevice, Window* window);
    ~MultiMeshRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;

    bool loadMeshes(const std::string& meshFile, const std::string& drawDataFile, const std::string& materialFile,
                    const std::string& vertexShaderFile, const std::string& fragmentShaderFile);

    void updateIndirectBuffers(u32 swapchainImageIndex, bool* visibility = nullptr);
    void updateGeometryBuffers(u32 vertexSize, u32 indexSize, const void* vertices, const void* indices);
    void updateMaterialBuffer(u32 materialSize, const void* materialData);

    void updateUniformBuffer(const mat4& mat);
    void updateDrawDataBuffer(u32 swapchainImageIndex, u32 drawDataSize, const void* drawData);
    void updateCountBuffer(u32 swapchainImageIndex, u32 itemCount);

    u32 VertexBufferSize;
    u32 IndexBufferSize;

private:
    bool createDescriptors();

    struct UniformBuffer
    {
        mat4 mvp;
    };

private:
    /*
     * GPU resources.
     */
    Buffer mStorageBuffer;
    Buffer mMaterialBuffer;
    std::vector<Buffer> mIndirectBuffers;
    std::vector<Buffer> mDrawDataBuffers;
    std::vector<Buffer> mDrawCountBuffers;

    /*
     * Vertex Data.
     */
    std::vector<DrawData> mDrawData;
    MeshData mMeshData;

    u32 mMaxVertexBufferSize{0};
    u32 mMaxIndexBufferSize{0};

    u32 mIndexBufferOffset{0}; // is eual to mMaxVertexbufferSize

    /*
     * Max size of draw data array.
     */
    u32 mMaxDrawDataCount{0};

    u32 mMaxDrawDataSize{0};
    u32 mMaxMaterialSize{0};
};

} // namespace Suoh
