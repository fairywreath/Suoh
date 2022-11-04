#include "MainRenderer.h"

#include <Logger.h>

/*
 * Window input callbacks to control camera
 */
FpsCameraWindowObserver::FpsCameraWindowObserver(Window& window)
    : mWindow(window), mCameraController(glm::vec3(0.0f, -5.0f, 15.0f), vec3(0.0f, 0.0f, -1.0f),
                                         vec3(0.0f, 1.0f, 0.0f))
{
    mWindow.AddObserver(*this);
}

void FpsCameraWindowObserver::OnCursorPos(double x, double y)
{
    auto width = mWindow.GetWidth();
    auto height = mWindow.GetHeight();

    mouseState.position.x = static_cast<float>(x / width);
    mouseState.position.y = static_cast<float>(y / height);
}

void FpsCameraWindowObserver::OnMouseButton(int key, int action, int mods)
{
    if (key == GLFW_MOUSE_BUTTON_LEFT)
        mouseState.pressedLeft = (action == GLFW_PRESS);
}

void FpsCameraWindowObserver::OnKey(int key, int scancode, int action, int mods)
{
    const bool pressed = (action != GLFW_RELEASE);

    if (key == GLFW_KEY_W)
    {
        mCameraController.Movement.forward = pressed;
        // LOG_INFO("w pressed!");
    }
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
        mWindow.Close();
}

void FpsCameraWindowObserver::OnResize(int width, int height)
{
}

void FpsCameraWindowObserver::OnScroll(double xoffset, double yoffset)
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

MainRenderer::MainRenderer(Window& window)
    : m_Window(window), mFpsCamWindowObserver(m_Window),
      mCamera(mFpsCamWindowObserver.getCameraController())
{
    DeviceDesc deviceDesc;
    deviceDesc.framebufferWidth = window.GetWidth();
    deviceDesc.framebufferHeight = window.GetHeight();
    deviceDesc.glfwWindowPtr = window.GetNativeWindow();
    m_Device = CreateVulkanDevice(deviceDesc);

    m_RenderDevice = std::make_unique<RenderDevice>(m_Device.get());

    m_SceneRenderer = std::make_unique<SceneRenderer>(m_RenderDevice.get(), m_Window);

    const auto swapchainImageCount = m_Device->GetSwapchainImageCount();
    m_CommandLists.resize(swapchainImageCount);
    for (int i = 0; i < swapchainImageCount; i++)
    {
        m_CommandLists[i] = m_Device->CreateCommandList({});
    }

    Init();
}

MainRenderer::~MainRenderer()
{
    m_Device->WaitIdle();
}

void MainRenderer::Render()
{
    m_Device->SwapchainAcquireNextImage();

    // Update after acquiring image.
    m_SceneRenderer->UpdateBuffers();

    auto imageIndex = m_Device->GetCurrentSwapchainImageIndex();
    auto& commandList = m_CommandLists[imageIndex];

    commandList->Begin();

    m_SceneRenderer->RecordCommands(commandList);

    commandList->End();

    m_Device->Submit(commandList);
    m_Device->Present();
}

void MainRenderer::Update(float deltaSeconds)
{
    mFpsCamWindowObserver.update(deltaSeconds);

    int width, height;
    glfwGetFramebufferSize((GLFWwindow*)m_Window.GetNativeWindow(), &width, &height);
    const float ratio = width / (float)height;

    const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
    const mat4 view = mCamera.getViewMatrix();

    m_SceneRenderer->SetMatrices(p, view);
    m_SceneRenderer->SetCameraPosition(mCamera.getPosition());
}

void MainRenderer::Init()
{
    LOG_INFO("Creating environment maps...");

    m_EnvMapTexture = MakeTexture(
        m_RenderDevice->CreateCubemapTextureImage("../Resources/piazza_bologni_1k.hdr"),
        m_Device->CreateSampler({}));
    m_IrrMapTexture = MakeTexture(
        m_RenderDevice->CreateCubemapTextureImage("../Resources/piazza_bologni_1k_irradiance.hdr"),
        m_Device->CreateSampler({}));

    LOG_INFO("Environment maps created.");

    LOG_INFO("Loading scene data....");

    m_SceneData.envMap = m_EnvMapTexture;
    m_SceneData.envMapIrradiance = m_IrrMapTexture;
    m_SceneData.framebufferWidth = m_Window.GetWidth();
    m_SceneData.framebufferHeight = m_Window.GetHeight();
    m_SceneData.Load("../Resources/Bistro/exterior.meshes", "../Resources/Bistro/exterior.scene",
                     "../Resources/Bistro/exterior.materials");
    m_SceneData.InitializeGPUResources(m_RenderDevice.get());

    LOG_INFO("Scene data loaded.");

    LOG_INFO("Creating scene data GPU resources...");

    m_SceneRenderer->Init(m_SceneData);

    LOG_INFO("Done creating scene data resources.");
}
