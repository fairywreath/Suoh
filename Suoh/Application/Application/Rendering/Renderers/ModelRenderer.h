#include "RendererBase.h"

namespace Suoh
{

class ModelRenderer : public RendererBase
{
public:
    ModelRenderer(VKRenderDevice* renderDevice, Window* window);
    ~ModelRenderer() override;

    void loadModel(const std::string& modelFile, const std::string& textureFile, size_t uniformDataSize);

    void recordCommands(VkCommandBuffer commandBuffer) override;
    void updateUniformBuffer(const void* data, size_t uniformDataSize);

private:
    void createDescriptors(size_t uniformDataSize);

private:
    size_t mVertexBufferSize;
    size_t mIndexBufferOffset; // offset of index buffer in SSBO
    size_t mIndexBufferSize;

    Buffer mStorageBuffer;

    Texture mTexture;
};

} // namespace Suoh