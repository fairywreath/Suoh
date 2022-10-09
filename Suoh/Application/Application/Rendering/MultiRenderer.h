#pragma once

#include <Core/SuohBase.h>

#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>
#include <Graphics/Scene/Material.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Scene/VertexData.h>

#include "Renderer.h"

namespace Suoh
{

struct SceneData
{
    SceneData(){};

    SceneData(VKRenderDevice& renderDevice, const std::string& meshFile, const std::string& sceneFile, const std::string& materialFile,
              Texture envMap, Texture irradianceMap);

    Texture envMapIrradiance;
    Texture envMap;
    Texture brdfLUT;

    Buffer materialsBuffer;
    Buffer transformsBuffer;

    VKRenderDevice* renderDevice;

    TextureArrayAttachment allMaterialTextures;

    BufferAttachment indexBuffer;
    BufferAttachment vertexBuffer;

    MeshData meshData;

    Scene::SceneContext scene;

    std::vector<MaterialDescription> materials;
    std::vector<std::string> textureFiles;

    std::vector<mat4> shapeTransforms;
    std::vector<DrawData> shapes;

    void loadMeshes(const std::string& meshFile);
    void loadScene(const std::string& sceneFile);

    void convertGlobalToShapeTransforms();
    void recalculateAllTransforms();
    void uploadGlobalTransforms();

    void updateMaterial(int matIdx);
};

class MultiRenderer : public Renderer
{
public:
    MultiRenderer(VKRenderDevice& renderDevice, SceneData& sceneData, const std::string& vertexShaderFile,
                  const std::string& fragmentShaderFile, const std::vector<Texture>& outputs = std::vector<Texture>{},
                  RenderPass screenRenderPass = RenderPass(),
                  const std::vector<BufferAttachment>& auxBuffers = std::vector<BufferAttachment>{},
                  const std::vector<TextureAttachment>& auxTextures = std::vector<TextureAttachment>{});

    SUOH_NON_COPYABLE(MultiRenderer);
    SUOH_NON_MOVEABLE(MultiRenderer);

    void fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) override;

    void updateBuffers() override;

    void updateIndirectBuffers(int imageIdx, bool* visibility = nullptr);

    inline void setMatrices(const glm::mat4& proj, const glm::mat4& view)
    {
        const glm::mat4 m1 = glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f));
        mUbo.proj = proj;
        mUbo.view = view * m1;
    }

    inline void setCameraPosition(const glm::vec3& cameraPos)
    {
        mUbo.cameraPos = glm::vec4(cameraPos, 1.0f);
    }

    inline const SceneData& getSceneData() const
    {
        return mSceneData;
    }

    inline Image getDepthImage()
    {
        return mDepthImage;
    }

private:
    SceneData& mSceneData;

    std::vector<Buffer> mIndirectBuffers;
    std::vector<Buffer> mShapes;

    struct UBO
    {
        mat4 proj;
        mat4 view;
        vec4 cameraPos;
    } mUbo;

    Image mDepthImage;
    std::vector<VkFramebuffer> mSwapchainFramebuffers;
};

} // namespace Suoh
