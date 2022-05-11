#include "WindowsWindow.h"

#include <Logger.h>

namespace Suou
{

WindowsWindow::WindowsWindow(const WindowProperties& props) : mWindow(nullptr), mWidth(0), mHeight(0), mTitle("")
{
    init(props);
}

WindowsWindow::~WindowsWindow()
{
    if (mWindow)
    {
        destroy();
    }
}

bool WindowsWindow::init(const WindowProperties& props)
{
    mWidth = props.width;
    mHeight = props.height;
    mTitle = props.title;

    // XXX: glfw state should not be tied to the window
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    mWindow = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);
    if (mWindow == nullptr)
    {
        LOG_ERROR("Failed to create window!");
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(mWindow, this);

    return true;
}

void WindowsWindow::destroy()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();
    mWindow = nullptr;
}

void WindowsWindow::close()
{
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void WindowsWindow::update()
{
    glfwSwapBuffers(mWindow);

    // XXX: detach events and window display
    glfwPollEvents();
}

bool WindowsWindow::shouldClose() const
{
    return glfwWindowShouldClose(mWindow);
}

void WindowsWindow::resize(u32 width, u32 height)
{
    glfwSetWindowSize(mWindow, width, height);
}

u32 WindowsWindow::getWidth() const
{
    return mWidth;
}

u32 WindowsWindow::getHeight() const
{
    return mHeight;
}

void* WindowsWindow::getNativeWindow() const
{
    return static_cast<void*>(mWindow);
}

} // namespace Suou
