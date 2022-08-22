#include "GuiRenderer.h"

#include <Core/Logger.h>

#include <glm/ext.hpp>

namespace Suoh
{

static constexpr u32 ImGuiVertexBufferSize = 512 * 1024 * sizeof(ImDrawVert);
static constexpr u32 ImGuiIndexBufferSize = 512 * 1024 * sizeof(u32);

GuiRenderer::GuiRenderer(VKRenderDevice* renderDevice, Window* window)
    : RendererBase(renderDevice, window, Image()),
      // this is pretty horrible actually, since we need to call ImGui::CreateContext() beforehand
      mImguiIO(ImGui::GetIO())
{
    init();
}

GuiRenderer::GuiRenderer(VKRenderDevice* renderDevice, Window* window, const std::vector<Texture>& textures)
    : RendererBase(renderDevice, window, Image()),
      mExtTextures(textures),
      mImguiIO(ImGui::GetIO())
{
    init();
}

GuiRenderer::~GuiRenderer()
{
    for (auto& buffer : mUniformBuffers)
        mRenderDevice->destroyBuffer(buffer);
    for (auto& buffer : mStorageBuffers)
        mRenderDevice->destroyBuffer(buffer);
    for (auto& frameBuffer : mSwapchainFramebuffers)
        mRenderDevice->destroyFramebuffer(frameBuffer);

    mRenderDevice->destroyTexture(mFontTexture);

    mRenderDevice->destroyRenderPass(mRenderPass);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyPipeline(mGraphicsPipeline);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);
}

void GuiRenderer::init()
{
    createFontTexture("resources/OpenSans-Light.ttf", mFontTexture.image);
    mRenderDevice->createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mFontTexture.image);
    mRenderDevice->createTextureSampler(mFontTexture);

    const auto imageCount = mRenderDevice->getSwapchainImageCount();
    mStorageBuffers.resize(imageCount);
    mBufferSize = ImGuiVertexBufferSize + ImGuiIndexBufferSize;

    for (auto& buffer : mStorageBuffers)
    {
        if (!mRenderDevice->createBuffer(mBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, buffer))
        {
            LOG_ERROR("GuiRenderer: create SBO buffer failed!");
            return;
        }
    }

    createUniformBuffers(sizeof(UniformBuffer));

    mRenderDevice->createColorDepthRenderPass({}, false, mRenderPass);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, VK_NULL_HANDLE, mSwapchainFramebuffers);

    createDescriptorSets();
    // createMultiDescriptorSets();

    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    // mRenderDevice->createPipelineLayoutWithConstants(mDescriptorSetLayout, mPipelineLayout, 0, sizeof(u32));

    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/Imgui.vert",
                                              "../../../Suoh/Shaders/Imgui.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mGraphicsPipeline,
                                          true, true, true);
}

void GuiRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    if (!mDrawData)
        return;

    beginRenderPass(commandBuffer);

    ImVec2 clipOff = mDrawData->DisplayPos;
    ImVec2 clipScale = mDrawData->FramebufferScale;

    int vertexOffset = 0;
    int indexOffset = 0;

    auto frameBufferWidth = mWindow->getWidth();
    auto frameBufferHeight = mWindow->getHeight();

    for (auto n = 0; n < mDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = mDrawData->CmdLists[n];
        for (int cmd = 0; cmd < cmdList->CmdBuffer.Size;
             cmd++)
        {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd];
            addImGuiItem(
                frameBufferWidth, frameBufferHeight,
                commandBuffer, pcmd, clipOff, clipScale,
                indexOffset, vertexOffset, mExtTextures, mPipelineLayout);
        }
        indexOffset += cmdList->IdxBuffer.Size;
        vertexOffset += cmdList->VtxBuffer.Size;
    }

    vkCmdEndRenderPass(commandBuffer);
}

void GuiRenderer::updateBuffers(const ImDrawData* imguiDrawData)
{
    mDrawData = imguiDrawData;

    const float L = mDrawData->DisplayPos.x;
    const float R = mDrawData->DisplayPos.x + mDrawData->DisplaySize.x;
    const float T = mDrawData->DisplayPos.y;
    const float B = mDrawData->DisplayPos.y + mDrawData->DisplaySize.y;

    UniformBuffer ubo{
        .inMatrix = glm::ortho(L, R, T, B),
    };

    auto currentImageIndex = mRenderDevice->getSwapchainImageIndex();
    mRenderDevice->uploadBufferData(mUniformBuffers[currentImageIndex], &ubo, sizeof(ubo));

    void* data = nullptr;
    mRenderDevice->mapMemory(mStorageBuffers[currentImageIndex].allocation, &data);

    ImDrawVert* vertex = (ImDrawVert*)data;
    for (auto n = 0; n < mDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = mDrawData->CmdLists[n];
        memcpy(vertex, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        vertex += cmdList->VtxBuffer.Size;
    }

    u32* index = (u32*)((u8*)data + ImGuiVertexBufferSize);
    for (auto n = 0; n < mDrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmdList = mDrawData->CmdLists[n];
        const uint16_t* src = (const u16*)cmdList->IdxBuffer.Data;
        for (int j = 0; j < cmdList->IdxBuffer.Size; j++)
            *index++ = (u32)*src++;
    }

    mRenderDevice->unmapMemory(mStorageBuffers[currentImageIndex].allocation);
}

bool GuiRenderer::createDescriptorSets()
{
    mRenderDevice->createDescriptorPool(1, 2, 1, mDescriptorPool);

    const std::vector<VkDescriptorSetLayoutBinding> bindings = {
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    };

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    const auto swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, mDescriptorSetLayout);

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = static_cast<u32>(swapchainImageCount),
        .pSetLayouts = layouts.data(),
    };

    mDescriptorSets.resize(swapchainImageCount);
    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    for (auto i = 0; i < swapchainImageCount; i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(UniformBuffer),
        };

        const VkDescriptorBufferInfo storageBufferVertexInfo = {
            .buffer = mStorageBuffers[i].buffer,
            .offset = 0,
            .range = ImGuiVertexBufferSize,
        };

        const VkDescriptorBufferInfo storageBufferIndexInfo = {
            .buffer = mStorageBuffers[i].buffer,
            .offset = ImGuiVertexBufferSize,
            .range = ImGuiIndexBufferSize,
        };

        const VkDescriptorImageInfo textureInfo = {
            .sampler = mFontTexture.sampler,
            .imageView = mFontTexture.image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        const std::vector<VkWriteDescriptorSet> descriptorWrites = {
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &uboInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferVertexInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferIndexInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &textureInfo},
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

bool GuiRenderer::createMultiDescriptorSets()
{
    return true;
}

bool GuiRenderer::createFontTexture(const std::string& fontFile, Image& textureImage)
{
    ImFontConfig cfg = ImFontConfig();
    // Build texture atlas   cfg.FontDataOwnedByAtlas = false;
    cfg.RasterizerMultiply = 1.5f;
    cfg.SizePixels = 768.0f / 32.0f;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 4;
    cfg.OversampleV = 4;
    ImFont* Font = mImguiIO.Fonts->AddFontFromFileTTF(fontFile.c_str(), cfg.SizePixels, &cfg);

    unsigned char* pixels = nullptr;
    int texWidth = 1, texHeight = 1;
    mImguiIO.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

    if (!pixels || !mRenderDevice->createTextureImageFromData(pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, textureImage))
    {
        return false;
    }

    mImguiIO.Fonts->TexID = (ImTextureID)0;
    mImguiIO.FontDefault = Font;
    mImguiIO.DisplayFramebufferScale = ImVec2(1, 1);

    return true;
}

void GuiRenderer::addImGuiItem(u32 width, u32 height, VkCommandBuffer commandBuffer, const ImDrawCmd* pcmd, ImVec2 clipOff, ImVec2 clipScale, int idxOffset, int vtxOffset,
                               const std::vector<Texture>& textures, VkPipelineLayout pipelineLayout)
{
    if (pcmd->UserCallback)
        return;

    // Project scissor/clipping rectangles into framebuffer space
    ImVec4 clipRect;
    clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
    clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
    clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
    clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

    if (clipRect.x < width && clipRect.y < height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
    {
        if (clipRect.x < 0.0f)
            clipRect.x = 0.0f;
        if (clipRect.y < 0.0f)
            clipRect.y = 0.0f;
        // Apply scissor/clipping rectangle
        const VkRect2D scissor = {
            .offset = {.x = (int32_t)(clipRect.x), .y = (int32_t)(clipRect.y)},
            .extent = {.width = (uint32_t)(clipRect.z - clipRect.x), .height = (uint32_t)(clipRect.w - clipRect.y)},
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // this is added in the Chapter 6: Using descriptor indexing in Vulkan to render an ImGui UI
        if (textures.size() > 0)
        {
            uint32_t texture = (uint32_t)(intptr_t)pcmd->TextureId;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), (const void*)&texture);
        }

        vkCmdDraw(commandBuffer,
                  pcmd->ElemCount,
                  1,
                  pcmd->IdxOffset + idxOffset,
                  pcmd->VtxOffset + vtxOffset);
    }
}

} // namespace Suoh