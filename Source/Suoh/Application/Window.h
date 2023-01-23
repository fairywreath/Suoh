#pragma once

#include <functional>
#include <string>
#include <vector>

#include <Core/Types.h>

struct WindowConfiguration
{
    u32 width{};
    u32 height{};
    std::string name;
};

// using WindowEventCallback = std::function<void(const void* event)>;

class Window final
{
public:
    explicit Window(const WindowConfiguration& configuration);
    ~Window();

    NON_COPYABLE(Window);
    NON_MOVEABLE(Window);

    void SetFullScreen(bool value);
    void CenterMouse(bool dragging);

    void HandleEvents();

    void* GetNativeWindowHandle() const
    {
        return m_NativeWindowHandle;
    }

    u32 GetWidth() const
    {
        return m_Width;
    }

    u32 GetHeight() const
    {
        return m_Height;
    }

    u32 GetRefreshRate() const
    {
        return m_RefreshRate;
    }

    bool Resized() const
    {
        return m_Resized;
    }

    bool Minimized() const
    {
        return m_Minimized;
    }

    bool ShouldClose() const
    {
        return m_ShouldClose;
    }

private:
    void* m_NativeWindowHandle{nullptr};

    std::string m_Name{""};

    u32 m_Width{0};
    u32 m_Height{0};
    f32 m_RefreshRate{1.0f / 60.0f};

    bool m_Resized{false};
    bool m_Minimized{false};
    bool m_ShouldClose{false};

    // std::vector<WindowEventCallback> m_WindowEventCallbacks;
};