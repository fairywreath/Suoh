#pragma once

#include <CoreTypes.h>

#include "RenderDevice.h"
#include "SceneData.h"
#include "Window/Window.h"

class SceneRenderer
{
public:
    SceneRenderer(RenderDevice* renderDevice, Window& window);

    NON_COPYABLE(SceneRenderer);
    NON_MOVEABLE(SceneRenderer);

    void Init(SceneData& sceneData);

    void RecordCommands(CommandListHandle commandList);

    void UpdateBuffers();

    inline void SetMatrices(const glm::mat4& proj, const glm::mat4& view)
    {
        const glm::mat4 m1 = glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f));
        m_Ubo.proj = proj;
        m_Ubo.view = view * m1;
    }

    inline void SetCameraPosition(const glm::vec3& cameraPos)
    {
        m_Ubo.cameraPos = glm::vec4(cameraPos, 1.0f);
    }

    inline ImageHandle GetDepthImage()
    {
        return m_DepthImage;
    }

private:
    void UpdateIndirectBuffers(int index, bool* visibility = nullptr);

    struct UBO
    {
        mat4 proj;
        mat4 view;
        vec4 cameraPos;
    } m_Ubo;

protected:
    RenderDevice* m_RenderDevice{nullptr};
    VulkanDevice* m_Device{nullptr};

    DescriptorLayoutDesc m_DescriptorLayoutDesc;
    DescriptorLayoutHandle m_DescriptorLayout;
    std::vector<DescriptorSetHandle> m_DescriptorSets;

    RenderPassHandle m_RenderPass;
    GraphicsPipelineHandle m_GraphicsPipeline;

    std::vector<BufferHandle> m_UniformBuffers;
    std::vector<BufferHandle> m_IndirectBuffers;
    std::vector<BufferHandle> m_Shapes;

    ImageHandle m_DepthImage;
    std::vector<FramebufferHandle> m_SwapchainFramebuffers;

    // Scene data.
    SceneData* m_SceneData;
    u32 m_FramebufferWidth;
    u32 m_FramebufferHeight;
};
