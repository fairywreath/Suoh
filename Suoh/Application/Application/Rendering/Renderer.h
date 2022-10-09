#pragma once

#include <Core/SuohBase.h>
#include <Core/Types.h>

#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>

namespace Suoh
{

class Renderer
{
public:
    Renderer(VKRenderDevice& renderDevice, u32 width, u32 height)
        : processingWidth(width), processingHeight(height), mRenderDevice(&renderDevice)
    {
    }

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, VkFramebuffer fb = VK_NULL_HANDLE, VkRenderPass rp = VK_NULL_HANDLE) = 0;

    virtual void updateBuffers()
    {
    }

    inline void updateUniformBuffer(const u32 offset, const u32 size, const void* data)
    {
        mRenderDevice->uploadBufferData(mUniformBuffers[mRenderDevice->getSwapchainImageIndex()], data, offset, size);
    }

    void initPipeline(const std::vector<std::string>& shaders, const GraphicsPipelineInfo& pipelineInfo, u32 vtxConstSize = 0,
                      u32 fragConstSize = 0)
    {
        mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
        mRenderDevice->createGraphicsPipeline(processingWidth, processingHeight, mRenderPass.handle, mPipelineLayout, shaders,
                                              pipelineInfo.topology, mGraphicsPipeline, pipelineInfo.useDepth, pipelineInfo.useBlending,
                                              pipelineInfo.dynamicScissorState);
    }

    GraphicsPipelineInfo initRenderPass(const GraphicsPipelineInfo& pipelineInfo, const std::vector<Texture>& outputs,
                                        RenderPass renderPass = RenderPass(), RenderPass fallbackPass = RenderPass())
    {
        GraphicsPipelineInfo outInfo = pipelineInfo;
        // if (!outputs.empty()) // offscreen rendering
        //{
        //     processingWidth = outputs[0].width;
        //     processingHeight = outputs[0].height;

        //    outInfo.width = processingWidth;
        //    outInfo.height = processingHeight;

        //    renderPass_ = (renderPass.handle != VK_NULL_HANDLE)
        //                      ? renderPass
        //                      : ((isDepthFormat(outputs[0].format) && (outputs.size() == 1))
        //                             ? ctx_.resources.addDepthRenderPass(outputs)
        //                             : ctx_.resources.addRenderPass(outputs, RenderPassCreateInfo(), true));
        //    framebuffer_ = ctx_.resources.addFramebuffer(renderPass_, outputs);
        //}
        // else
        //{
        //    renderPass_ = (renderPass.handle != VK_NULL_HANDLE) ? renderPass : fallbackPass;
        mRenderPass = (renderPass.handle != VK_NULL_HANDLE) ? renderPass : fallbackPass;
        //}
        return outInfo;
    }

    void beginRenderPass(VkRenderPass rp, VkFramebuffer fb, VkCommandBuffer commandBuffer)
    {
        const auto currentImage = mRenderDevice->getSwapchainImageIndex();

        const VkClearValue clearValues[2] = {
            VkClearValue{.color = {1.0f, 1.0f, 1.0f, 1.0f}},
            VkClearValue{.depthStencil = {1.0f, 0}},
        };

        const VkRect2D rect{
            .offset = {0, 0},
            .extent = {.width = processingWidth, .height = processingHeight},
        };

        const VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = rp,
            .framebuffer = fb,
            .renderArea = rect,
            .clearValueCount = 2,
            .pClearValues = &clearValues[0],
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mDescriptorSets[currentImage], 0,
                                nullptr);
    }

    VkFramebuffer Framebuffer{nullptr};
    RenderPass mRenderPass;

    u32 processingWidth;
    u32 processingHeight;

    // Updating individual textures (9 is the binding in our Chapter7-Chapter9 IBL scene shaders)
    void updateTexture(u32 textureIndex, Texture texture, u32 bindingIndex = 9)
    {
        for (auto ds : mDescriptorSets)
        {
            const VkDescriptorImageInfo imageInfo = {
                .sampler = texture.sampler,
                .imageView = texture.image.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkWriteDescriptorSet writeSet = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = ds,
                .dstBinding = bindingIndex,
                .dstArrayElement = textureIndex,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(mRenderDevice->getVkDevice(), 1, &writeSet, 0, nullptr);
        }
    }

protected:
    VKRenderDevice* mRenderDevice{nullptr};

    VkDescriptorSetLayout mDescriptorSetLayout{nullptr};
    VkDescriptorPool mDescriptorPool{nullptr};
    std::vector<VkDescriptorSet> mDescriptorSets;

    VkPipelineLayout mPipelineLayout{nullptr};
    VkPipeline mGraphicsPipeline{nullptr};

    std::vector<Buffer> mUniformBuffers;
};

} // namespace Suoh