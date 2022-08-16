#include "MainRenderer.h"

#include <Logger.h>

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace Suou
{

void MainRenderer::init(Window* window)
{
    mWindow = window;
    mRenderDevice = std::make_unique<VKRenderDevice>(window);

    mModelRenderer = std::make_unique<ModelRenderer>(mRenderDevice.get(), mWindow);
    mModelRenderer->loadModel("Resources/rubber_duck/scene.gltf", "resources/rubber_duck/textures/Duck_baseColor.png", sizeof(UniformBuffer));

    mClearRenderer = std::make_unique<ClearRenderer>(mRenderDevice.get(), mWindow, mModelRenderer->getDepthImage());
    mFinishRenderer = std::make_unique<FinishRenderer>(mRenderDevice.get(), mWindow, mModelRenderer->getDepthImage());

    mRenderers = {
        mClearRenderer.get(),
        mModelRenderer.get(),
        mFinishRenderer.get(),
    };
}

void MainRenderer::render()
{
    mRenderDevice->swapchainAcquireNextImage();

    mRenderDevice->resetCommandPool();

    composeFrame();

    mRenderDevice->submit(mRenderDevice->getCurrentCommandBuffer());

    mRenderDevice->present();

    mRenderDevice->deviceWaitIdle();
}

void MainRenderer::update()
{
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

    const UniformBuffer ubo = {
        .mvp = p * m1,
    };

    mModelRenderer->updateUniformBuffer(&ubo, sizeof(ubo));
}

void MainRenderer::composeFrame()
{
    update();

    VkCommandBuffer commandBuffer = mRenderDevice->getCurrentCommandBuffer();
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    for (auto& renderer : mRenderers)
    {
        renderer->recordCommands(commandBuffer);
    }

    vkEndCommandBuffer(commandBuffer);
}

} // namespace Suou