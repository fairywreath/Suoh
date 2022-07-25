#include "MainRenderer.h"

#include <Logger.h>

#include <array>

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace Suou
{

MainRenderer::MainRenderer()
    : mRenderDevice(nullptr),
      mWindow(nullptr)
{
}

MainRenderer::~MainRenderer()
{
    destroy();
}

void MainRenderer::init(Window* window)
{
    mRenderDevice = std::make_unique<VKRenderDevice>(window);
    mWindow = window;

    // create SSBO and UBOs
    mRenderDevice->createTexturedVertexBuffer("Resources/rubber_duck/scene.gltf", mStorageBuffer, mVertexBufferSize, mIndexBufferSize, mIndexBufferOffset);
    createUniformBuffers();

    // create color attachment texture
    mRenderDevice->createTextureImage("resources/rubber_duck/textures/Duck_baseColor.png", mTexture);
    mRenderDevice->createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mTexture.image);
    mRenderDevice->createTextureSampler(mTexture);

    // create depth image
    mRenderDevice->createDepthResources(mWindow->getWidth(), mWindow->getHeight(), mDepthImage);

    // initialize descriptor sets/layout
    createDescriptors();
    updateDescriptors();

    // initialize graphics pipeline
    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = true,
            .clearDepth = true,
            .flags = eRenderPassBit::eRenderPassBitFirst | eRenderPassBitLast,
        },
        true, mRenderPass);

    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "Shaders/Simple.vert",
                                              //   "Shaders/VK02.geom",
                                              "Shaders/Simple.frag",
                                          },
                                          mGraphicsPipeline);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

void MainRenderer::destroy()
{
    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }

    mRenderDevice->destroyRenderPass(mRenderPass);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyPipeline(mGraphicsPipeline);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);

    mRenderDevice->destroyImage(mDepthImage);
    mRenderDevice->destroyTexture(mTexture);

    // destroy uniform buffers
    for (auto& buffer : mUniformBuffers)
    {
        mRenderDevice->destroyBuffer(buffer);
    }

    mRenderDevice->destroyBuffer(mStorageBuffer);

    mRenderDevice->destroy();
    mRenderDevice.release();
}

void MainRenderer::render()
{
    mRenderDevice->swapchainAcquireNextImage();

    mRenderDevice->resetCommandPool();

    /*
     * misc. matrix calculations
     */
    int width, height;
    glfwGetFramebufferSize((GLFWwindow*)mWindow->getNativeWindow(), &width, &height);
    const float ratio = width / (float)height;

    const mat4 m1 = glm::rotate(
        glm::translate(mat4(1.0f), vec3(0.f, 0.5f, -1.5f)) * glm::rotate(mat4(1.f), glm::pi<float>(), vec3(1, 0, 0)),
        (float)glfwGetTime(),
        vec3(0.0f, 1.0f, 0.0f));
    const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

    const UniformBuffer ubo{.mvp = p * m1};

    updateUniformBuffer(mRenderDevice->getSwapchainImageIndex(), ubo);

    fillCommandBuffer();

    mRenderDevice->submit(mRenderDevice->getCurrentCommandBuffer());

    mRenderDevice->present();

    mRenderDevice->deviceWaitIdle();
}

bool MainRenderer::fillCommandBuffer()
{
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };

    const std::array<VkClearValue, 2> clearValues = {
        VkClearValue{.color = ClearValueColor},
        VkClearValue{.depthStencil = {1.0f, 0}},
    };

    const VkRect2D screenRect = {
        .offset = {0, 0},
        .extent = {
            .width = mWindow->getWidth(),
            .height = mWindow->getHeight(),
        },
    };

    VkCommandBuffer commandBuffer = mRenderDevice->getCurrentCommandBuffer();
    size_t swapchainImageIdx = mRenderDevice->getSwapchainImageIndex();

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = mRenderPass,
        // XXX: get directly from swapchain?
        .framebuffer = mSwapchainFramebuffers[swapchainImageIdx],
        .renderArea = screenRect,
        .clearValueCount = (u32)clearValues.size(),
        .pClearValues = clearValues.data(),
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1,
                            &mDescriptorSets[swapchainImageIdx], 0, nullptr);

    vkCmdDraw(commandBuffer, (u32)(mIndexBufferSize / sizeof(u32)), 1, 0, 0);
    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return true;
}

bool MainRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBuffer);

    mUniformBuffers.resize(mRenderDevice->getSwapchainImageCount());

    for (int i = 0; i < mUniformBuffers.size(); i++)
    {
        Buffer& buffer = mUniformBuffers[i];
        if (!mRenderDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VMA_MEMORY_USAGE_GPU_ONLY, buffer))
        {
            return false;
        }
    }

    return true;
}

void MainRenderer::updateUniformBuffer(u32 imageIndex, const UniformBuffer& ubo)
{
    Buffer& buffer = mUniformBuffers[imageIndex];
    void* data = nullptr;

    mRenderDevice->mapMemory(buffer.allocation, &data);
    memcpy(data, &ubo, sizeof(ubo));
    mRenderDevice->unmapMemory(buffer.allocation);
}

bool MainRenderer::createDescriptors()
{
    mRenderDevice->createDescriptorPool(1, 2, 1, mDescriptorPool);

    const std::vector<VkDescriptorSetLayoutBinding>
        bindings
        = {
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

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    u32 swapchainImageCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, mDescriptorSetLayout);

    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    return true;
}

void MainRenderer::updateDescriptors()
{
    for (size_t i = 0; i < mRenderDevice->getSwapchainImageCount(); i++)
    {
        const VkDescriptorBufferInfo uboInfo = {
            .buffer = mUniformBuffers[i].buffer,
            .offset = 0,
            .range = sizeof(UniformBuffer),
        };
        const VkDescriptorBufferInfo storageBufferVertexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = 0,
            .range = mVertexBufferSize,
        };
        const VkDescriptorBufferInfo storageBufferIndexInfo = {
            .buffer = mStorageBuffer.buffer,
            .offset = mIndexBufferOffset,
            .range = mIndexBufferSize,
        };
        const VkDescriptorImageInfo textureInfo = {
            .sampler = mTexture.sampler,
            .imageView = mTexture.image.imageView,
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
                .pBufferInfo = &uboInfo},
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferVertexInfo},
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = mDescriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &storageBufferIndexInfo},
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
}

} // namespace Suou