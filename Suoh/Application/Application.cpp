#include "Application.h"

#include <Logger.h>
#include <PlatformDefines.h>

#include <RenderLib/Vulkan/VKRenderDevice.h>

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
 * Main Application
 */
Application::Application()
    : mIsInitialized(false),
      mMainWindow(WindowProperties{
          .width = 1280,
          .height = 720,
          .title = "Suoh Engine",
      }),
      mFpsCamWindowObserver(mMainWindow),
      mCamera(mFpsCamWindowObserver.getCameraController())
{
    init();
}

Application::~Application()
{
    if (mIsInitialized)
    {
        destroy();
    }
}

void Application::run()
{
    if (!mIsInitialized)
    {
        return;
    }

    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    // display window
    while (!mMainWindow.shouldClose())
    {
        mFpsCamWindowObserver.update(deltaSeconds);
        mRenderer.render();

        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        mMainWindow.update();
    }
}

void Application::init()
{
    mRenderer.init(&mMainWindow);
    mRenderer.setCamera(mCamera);

    mIsInitialized = true;
}

void Application::destroy()
{
    mMainWindow.destroy();
}

} // namespace Suoh
