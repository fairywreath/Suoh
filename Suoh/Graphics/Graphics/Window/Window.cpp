#include "Window.h"

#include <Core/Logger.h>

namespace Suoh
{

Window::Window(const WindowProperties& props)
{
    init(props);
}

Window::~Window()
{
    if (mWindow)
    {
        destroy();
    }
}

bool Window::init(const WindowProperties& props)
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

    glfwSetCursorPosCallback(mWindow, onCursorPosCallback);
    glfwSetMouseButtonCallback(mWindow, onMouseButtonCallback);
    glfwSetKeyCallback(mWindow, onKeyCallback);
    glfwSetFramebufferSizeCallback(mWindow, onResizeCallback);
    glfwSetScrollCallback(mWindow, onScrollCallback);

    LOG_INFO("finished init window");

    return true;
}

void Window::destroy()
{
    glfwDestroyWindow(mWindow);
    glfwTerminate();

    mWindow = nullptr;
}

void Window::close()
{
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void Window::update()
{
    glfwSwapBuffers(mWindow);

    // XXX: detach events and window display
    glfwPollEvents();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(mWindow);
}

void Window::resize(u32 width, u32 height)
{
    glfwSetWindowSize(mWindow, width, height);
}

u32 Window::getWidth() const
{
    return mWidth;
}

u32 Window::getHeight() const
{
    return mHeight;
}

void* Window::getNativeWindow() const
{
    return static_cast<void*>(mWindow);
}

void Window::onCursorPos(double x, double y)
{
    for (auto observer : mObservers)
    {
        observer->onCursorPos(x, y);
    }
}

void Window::onMouseButton(int key, int action, int mods)
{
    for (auto observer : mObservers)
    {
        observer->onMouseButton(key, action, mods);
    }
}

void Window::onKey(int key, int scancode, int action, int mods)
{
    for (auto observer : mObservers)
    {
        observer->onKey(key, scancode, action, mods);
    }
}

void Window::onResize(int width, int height)
{
    for (auto observer : mObservers)
    {
        observer->onResize(width, height);
    }
}

void Window::onScroll(double xOffset, double yOffset)
{
    for (auto observer : mObservers)
    {
        observer->onScroll(xOffset, yOffset);
    }
}

void Window::onCursorPosCallback(GLFWwindow* window, double x, double y)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->onCursorPos(x, y);
}

void Window::onMouseButtonCallback(GLFWwindow* window, int key, int action, int mods)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->onMouseButton(key, action, mods);
}

void Window::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->onKey(key, scancode, action, mods);
}

void Window::onResizeCallback(GLFWwindow* window, int width, int height)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->onResize(width, height);
}

void Window::onScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->onScroll(xOffset, yOffset);
}

void Window::addObserver(WindowObserver& observer)
{
    mObservers.push_back(&observer);
}

void Window::removeObserver(WindowObserver& observer)
{
    auto found = std::find_if(mObservers.begin(), mObservers.end(),
                              [&](WindowObserver* p) -> bool { return p == &observer; });

    assert(found != mObservers.end());

    mObservers.erase(found);
}

} // namespace Suoh
