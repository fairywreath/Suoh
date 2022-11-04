#pragma once

#include <GLFW/glfw3.h>

#include <CoreTypes.h>

#include <string>
#include <vector>

class WindowObserver
{
public:
    virtual void OnCursorPos(double x, double y) = 0;
    virtual void OnMouseButton(int key, int action, int mods) = 0;
    virtual void OnKey(int key, int scancode, int action, int mods) = 0;
    virtual void OnResize(int width, int height) = 0;
    virtual void OnScroll(double xOffset, double yOffset) = 0;
};

struct WindowProperties
{
    u32 width{0};
    u32 height{0};
    std::string title{""};
};

struct Resolution
{
    u32 width{0};
    u32 height{0};
};

class Window
{
public:
    explicit Window(const WindowProperties& props);
    ~Window();

    NON_COPYABLE(Window);
    NON_MOVEABLE(Window);

    bool Init(const WindowProperties& props);
    void Destroy();
    void Close();
    void Update();

    bool ShouldClose() const;
    void Resize(u32 width, u32 height);

    u32 GetWidth() const;
    u32 GetHeight() const;
    void* GetNativeWindow() const;

    void AddObserver(WindowObserver& observer);
    void RemoveObserver(WindowObserver& observer);

private:
    void OnCursorPos(double x, double y);
    void OnMouseButton(int key, int action, int mods);
    void OnKey(int key, int scancode, int action, int mods);
    void OnResize(int width, int height);
    void OnScroll(double xOffset, double yOffset);

    static void OnCursorPosCallback(GLFWwindow* window, double x, double y);
    static void OnMouseButtonCallback(GLFWwindow* window, int key, int action, int mods);
    static void OnKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void OnResizeCallback(GLFWwindow* window, int width, int height);
    static void OnScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    Resolution DetectResolution();

private:
    GLFWwindow* m_Window{nullptr};

    u32 m_Width{0};
    u32 m_Height{0};
    std::string m_Title{""};

    std::vector<WindowObserver*> m_Observers;
};
