#pragma once

// XXX: glfw only supports vulkan and ogl?
#include <GLFW/glfw3.h>

#include <Window.h>

namespace Suou
{

class WindowsWindow final : public Window
{
public:
    explicit WindowsWindow(const WindowProperties& props);
    ~WindowsWindow() override;

    bool init(const WindowProperties& props) override final;
    void destroy() override final;
    void close() override final;
    void update() override final;

    bool shouldClose() const override final;
    void resize(u32 width, u32 height) override final;

    u32 getWidth() const override final;
    u32 getHeight() const override final;

    void* getNativeWindow() const override final;

private:
    GLFWwindow* mWindow;
    
    u32 mWidth;
    u32 mHeight;
    std::string mTitle;
};

}

