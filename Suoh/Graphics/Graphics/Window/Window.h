#pragma once

#include <GLFW/glfw3.h>

#include <Core/SuohBase.h>
#include <Core/Types.h>

#include <string>
#include <vector>

namespace Suoh
{

class WindowObserver
{
public:
    virtual void onCursorPos(double x, double y) = 0;
    virtual void onMouseButton(int key, int action, int mods) = 0;
    virtual void onKey(int key, int scancode, int action, int mods) = 0;
    virtual void onResize(int width, int height) = 0;
    virtual void onScroll(double xOffset, double yOffset) = 0;
};

struct WindowProperties
{
    u32 width{0};
    u32 height{0};
    std::string title{""};
};

class Window
{
public:
    explicit Window(const WindowProperties& props);
    ~Window();

    SUOH_NON_COPYABLE(Window);
    SUOH_NON_MOVEABLE(Window);

    bool init(const WindowProperties& props);
    void destroy();
    void close();
    void update();

    bool shouldClose() const;
    void resize(u32 width, u32 height);

    u32 getWidth() const;
    u32 getHeight() const;
    void* getNativeWindow() const;

    void addObserver(WindowObserver& observer);
    void removeObserver(WindowObserver& observer);

private:
    void onCursorPos(double x, double y);
    void onMouseButton(int key, int action, int mods);
    void onKey(int key, int scancode, int action, int mods);
    void onResize(int width, int height);
    void onScroll(double xOffset, double yOffset);

    static void onCursorPosCallback(GLFWwindow* window, double x, double y);
    static void onMouseButtonCallback(GLFWwindow* window, int key, int action, int mods);
    static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onResizeCallback(GLFWwindow* window, int width, int height);
    static void onScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

private:
    GLFWwindow* mWindow{nullptr};

    u32 mWidth{0};
    u32 mHeight{0};
    std::string mTitle{""};

    std::vector<WindowObserver*> mObservers;
};

} // namespace Suoh