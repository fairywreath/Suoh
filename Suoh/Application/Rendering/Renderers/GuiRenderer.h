#pragma once

#include "RendererBase.h"

#include <imgui.h>

namespace Suoh
{

class GuiRenderer : public RendererBase
{
public:
    GuiRenderer(VKRenderDevice* renderDevice, Window* window);
    GuiRenderer(VKRenderDevice* renderDevice, Window* window, const std::vector<Texture>& textures);
    ~GuiRenderer() override;

    void recordCommands(VkCommandBuffer commandBuffer) override;
    void updateBuffers(const ImDrawData* imguiDrawData);

    void addImGuiItem(u32 width, u32 height, VkCommandBuffer commandBuffer, const ImDrawCmd* pcmd, ImVec2 clipOff, ImVec2 clipScale, int idxOffset, int vtxOffset,
                      const std::vector<Texture>& textures, VkPipelineLayout pipelineLayout);

private:
    bool createDescriptorSets();
    bool createMultiDescriptorSets(); // descriptor set with multiple textures

    bool createFontTexture(const std::string& fontFile, Image& textureImage);

    void init();

    struct UniformBuffer
    {
        mat4 inMatrix;
    };

private:
    const ImDrawData* mDrawData{nullptr};
    ImGuiIO& mImguiIO;

    std::vector<Texture> mExtTextures;

    size_t mBufferSize;
    std::vector<Buffer> mStorageBuffers;

    // VkSampler mFontSampler;
    // Image mFontImage{};

    Texture mFontTexture{};
};

} // namespace Suoh