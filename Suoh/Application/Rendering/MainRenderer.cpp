#include "MainRenderer.h"

#include <Logger.h>

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace Suoh
{

/*
 * Window input callbacks to control camera
 */
FpsCameraWindowObserver::FpsCameraWindowObserver(Window& window)
    : mWindow(window)
{
    mWindow.addObserver(*this);
}

void FpsCameraWindowObserver::onCursorPos(double x, double y)
{
    auto width = mWindow.getWidth();
    auto height = mWindow.getHeight();

    mouseState.position.x = static_cast<float>(x / width);
    mouseState.position.y = static_cast<float>(y / height);
}

void FpsCameraWindowObserver::onMouseButton(int key, int action, int mods)
{
    if (key == GLFW_MOUSE_BUTTON_LEFT)
        mouseState.pressedLeft = (action == GLFW_PRESS);
}

void FpsCameraWindowObserver::onKey(int key, int scancode, int action, int mods)
{
    const bool pressed = (action != GLFW_RELEASE);

    if (key == GLFW_KEY_W)
        mCameraController.Movement.forward = pressed;
    if (key == GLFW_KEY_S)
        mCameraController.Movement.backward = pressed;
    if (key == GLFW_KEY_A)
        mCameraController.Movement.left = pressed;
    if (key == GLFW_KEY_D)
        mCameraController.Movement.right = pressed;
    if (key == GLFW_KEY_1)
        mCameraController.Movement.up = pressed;
    if (key == GLFW_KEY_2)
        mCameraController.Movement.down = pressed;
    if (mods & GLFW_MOD_SHIFT)
        mCameraController.Movement.fast = pressed;
    if (key == GLFW_KEY_SPACE)
        mCameraController.setUpVector(vec3(0.0f, 1.0f, 0.0f));

    // XXX: not a camera functionality, esc to exit
    if (key == GLFW_KEY_ESCAPE && pressed)
        mWindow.close();
}

void FpsCameraWindowObserver::onResize(int width, int height)
{
}

void FpsCameraWindowObserver::onScroll(double xoffset, double yoffset)
{
}

void FpsCameraWindowObserver::update(float deltaSeconds)
{
    mCameraController.update(deltaSeconds, mouseState.position, mouseState.pressedLeft);
}

FirstPersonCameraController& FpsCameraWindowObserver::getCameraController()
{
    return mCameraController;
}

/*
 * Main renderer
 */
MainRenderer::MainRenderer(Window& window)
    : mWindow(window),
      mFpsCamWindowObserver(mWindow),
      mCamera(mFpsCamWindowObserver.getCameraController())
{
    init();
}

void MainRenderer::init()
{
    mRenderDevice = std::make_unique<VKRenderDevice>(&mWindow);

    mModelRenderer = std::make_unique<ModelRenderer>(mRenderDevice.get(), &mWindow);
    mModelRenderer->loadModel("Resources/rubber_duck/scene.gltf", "Resources/rubber_duck/textures/Duck_baseColor.png", sizeof(ModelUniformBuffer));

    mClearRenderer = std::make_unique<ClearRenderer>(mRenderDevice.get(), &mWindow, mModelRenderer->getDepthImage());
    mFinishRenderer = std::make_unique<FinishRenderer>(mRenderDevice.get(), &mWindow, mModelRenderer->getDepthImage());
    mCanvasRenderer = std::make_unique<CanvasRenderer>(mRenderDevice.get(), &mWindow, mModelRenderer->getDepthImage());

    ImGui::CreateContext();
    mGuiRenderer = std::make_unique<GuiRenderer>(mRenderDevice.get(), &mWindow);

    mRenderers = {
        mClearRenderer.get(),
        mModelRenderer.get(),
        mCanvasRenderer.get(),
        mGuiRenderer.get(),
        mFinishRenderer.get(),
    };

    // mCanvasRenderer->drawLine({-1.0f, -1.0f, 0.5f}, {1.0f, 1.0f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f});
    // mCanvasRenderer->updateBuffer();
}

void MainRenderer::render(float deltaSeconds)
{
    mRenderDevice->swapchainAcquireNextImage();

    mRenderDevice->resetCommandPool();

    update(deltaSeconds);
    composeFrame();

    mRenderDevice->submit(mRenderDevice->getCurrentCommandBuffer());

    mRenderDevice->present();

    mRenderDevice->deviceWaitIdle();

    mFpsCounter.tick(deltaSeconds);

    renderGui();
}

void MainRenderer::update(float deltaSeconds)
{
    // XXX: update sselected cameta only?
    mFpsCamWindowObserver.update(deltaSeconds);
    mFreeMovingCameraController.update(deltaSeconds);

    /*
     * misc. matrix calculations
     */
    int width, height;
    glfwGetFramebufferSize((GLFWwindow*)mWindow.getNativeWindow(), &width, &height);
    const float ratio = width / (float)height;

    const mat4 m1 = glm::rotate(
        glm::translate(mat4(1.0f), vec3(0.f, 0.5f, -1.5f)) * glm::rotate(mat4(1.f), glm::pi<float>(), vec3(1, 0, 0)),
        (float)glfwGetTime(),
        vec3(0.0f, 1.0f, 0.0f));
    const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

    const mat4 view = mCamera.getViewMatrix();

    const ModelUniformBuffer ubo = {
        .mvp = p * view * m1,
        // .mvp = p * m1,
    };

    mModelRenderer->updateUniformBuffer(&ubo, sizeof(ubo));
    // mCanvasRenderer->updateUniformBuffer(ubo.mvp, 0);
}

void MainRenderer::composeFrame()
{
    renderGui();

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

namespace
{
// ImGUI stuff
const char* cameraType = "First Person";
const char* comboBoxItems[] = {"First Person", "Free Moving"};
const char* currentComboBoxItem = cameraType;

} // namespace

void MainRenderer::renderGui()
{
    auto width = mWindow.getWidth();
    auto height = mWindow.getHeight();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    ImGui::NewFrame();

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Statistics", nullptr, flags);
    ImGui::TextColored(ImVec4(0, 0, 0, 1), "FPS: %.3f", mFpsCounter.getFps());
    ImGui::End();

    ImGui::Begin("Camera Control", nullptr);
    {
        if (ImGui::BeginCombo("##combo", currentComboBoxItem)) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < IM_ARRAYSIZE(comboBoxItems); n++)
            {
                const bool isSelected = (currentComboBoxItem == comboBoxItems[n]);

                if (ImGui::Selectable(comboBoxItems[n], isSelected))
                    currentComboBoxItem = comboBoxItems[n];

                if (isSelected)
                    ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }

        if (!strcmp(cameraType, "Free Moving"))
        {
            if (ImGui::SliderFloat3("Position", glm::value_ptr(mFreeMovingCameraPos), -10.0f, +10.0f))
                mFreeMovingCameraController.setDesiredPosition(mFreeMovingCameraPos);
            if (ImGui::SliderFloat3("Pitch/Pan/Roll", glm::value_ptr(mFreeMovingCameraAngles), -90.0f, +90.0f))
                mFreeMovingCameraController.setDesiredAngles(mFreeMovingCameraAngles);
        }

        if (currentComboBoxItem && strcmp(currentComboBoxItem, cameraType))
        {
            LOG_INFO("Selected new camera type: ", currentComboBoxItem);
            cameraType = currentComboBoxItem;
            reinitCamera();
        }
    }
    ImGui::End();

    ImGui::Render();
    mGuiRenderer->updateBuffers(ImGui::GetDrawData());
}

void MainRenderer::reinitCamera()
{

    if (!strcmp(cameraType, "First Person"))
    {
        mCamera = Camera(mFpsCamWindowObserver.getCameraController());
    }
    else
    {
        if (!strcmp(cameraType, "Free Moving"))
        {
            mFreeMovingCameraController.setDesiredPosition(mFreeMovingCameraPos);
            mFreeMovingCameraController.setDesiredAngles(mFreeMovingCameraAngles.x, mFreeMovingCameraAngles.y, mFreeMovingCameraAngles.z);
            mCamera = Camera(mFreeMovingCameraController);
        }
    }
}

} // namespace Suoh
