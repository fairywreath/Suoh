#include "Window.h"

#include <Logger.h>

Window::Window(const WindowProperties& props)
{
    Init(props);
}

Window::~Window()
{
    if (m_Window)
    {
        Destroy();
    }
}

bool Window::Init(const WindowProperties& props)
{
    m_Width = props.width;
    m_Height = props.height;
    m_Title = props.title;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // const auto resolution = detectResolution();
    // m_Width = resolution.width;
    // mHeight = resolution.height;

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), nullptr, nullptr);
    if (m_Window == nullptr)
    {
        LOG_ERROR("Failed to create window!");
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetCursorPosCallback(m_Window, OnCursorPosCallback);
    glfwSetMouseButtonCallback(m_Window, OnMouseButtonCallback);
    glfwSetKeyCallback(m_Window, OnKeyCallback);
    glfwSetFramebufferSizeCallback(m_Window, OnResizeCallback);
    glfwSetScrollCallback(m_Window, OnScrollCallback);

    return true;
}

void Window::Destroy()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();

    m_Window = nullptr;
}

void Window::Close()
{
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

void Window::Update()
{
    glfwPollEvents();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}

void Window::Resize(u32 width, u32 height)
{
    glfwSetWindowSize(m_Window, width, height);
}

u32 Window::GetWidth() const
{
    return m_Width;
}

u32 Window::GetHeight() const
{
    return m_Height;
}

void* Window::GetNativeWindow() const
{
    return static_cast<void*>(m_Window);
}

void Window::OnCursorPos(double x, double y)
{
    for (auto observer : m_Observers)
    {
        observer->OnCursorPos(x, y);
    }
}

void Window::OnMouseButton(int key, int action, int mods)
{
    for (auto observer : m_Observers)
    {
        observer->OnMouseButton(key, action, mods);
    }
}

void Window::OnKey(int key, int scancode, int action, int mods)
{
    for (auto observer : m_Observers)
    {
        observer->OnKey(key, scancode, action, mods);
    }
}

void Window::OnResize(int width, int height)
{
    for (auto observer : m_Observers)
    {
        observer->OnResize(width, height);
    }
}

void Window::OnScroll(double xOffset, double yOffset)
{
    for (auto observer : m_Observers)
    {
        observer->OnScroll(xOffset, yOffset);
    }
}

void Window::OnCursorPosCallback(GLFWwindow* window, double x, double y)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->OnCursorPos(x, y);
}

void Window::OnMouseButtonCallback(GLFWwindow* window, int key, int action, int mods)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->OnMouseButton(key, action, mods);
}

void Window::OnKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->OnKey(key, scancode, action, mods);
}

void Window::OnResizeCallback(GLFWwindow* window, int width, int height)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->OnResize(width, height);
}

void Window::OnScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    static_cast<Window*>(glfwGetWindowUserPointer(window))->OnScroll(xOffset, yOffset);
}

void Window::AddObserver(WindowObserver& observer)
{
    m_Observers.push_back(&observer);
}

void Window::RemoveObserver(WindowObserver& observer)
{
    auto found = std::find_if(m_Observers.begin(), m_Observers.end(),
                              [&](WindowObserver* p) -> bool { return p == &observer; });

    assert(found != m_Observers.end());

    m_Observers.erase(found);
}

Resolution Window::DetectResolution()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const int code = glfwGetError(nullptr);

    if (code != 0)
    {
        return Resolution{};
    }

    const GLFWvidmode* info = glfwGetVideoMode(monitor);

    const u32 windowW = (u32)info->width;
    const u32 windowH = (u32)info->height;

    return Resolution{
        .width = windowW,
        .height = windowH,
    };
}
