#include "Window.h"

#include <cassert>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl.h>

#include <Core/Logger.h>

namespace
{

f32 GetMonitorRefreshRate()
{
    SDL_DisplayMode displayMode;
    auto res = SDL_GetCurrentDisplayMode(0, &displayMode);
    assert(res == 0);
    return 1.0f / displayMode.refresh_rate;
}

inline SDL_Window* GetSDLWindowPtr(void* handle)
{
    return static_cast<SDL_Window*>(handle);
}

} // namespace

Window::Window(const WindowConfiguration& configuration)
{
    LOG_TRACE("Initializing window...");

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        LOG_ERROR("SDL Init Error: ", SDL_GetError());
        return;
    }

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);

    SDL_WindowFlags window_flags
        = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    auto windowHandle = SDL_CreateWindow(configuration.name.c_str(), SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED, configuration.width,
                                         configuration.height, window_flags);

    int width;
    int height;
    SDL_Vulkan_GetDrawableSize(windowHandle, &width, &height);

    m_NativeWindowHandle = windowHandle;
    m_Width = static_cast<u32>(width);
    m_Height = static_cast<u32>(height);
    m_RefreshRate = GetRefreshRate();

    LOG_TRACE("Done initializing window.");
}

Window::~Window()
{
    SDL_DestroyWindow(GetSDLWindowPtr(m_NativeWindowHandle));
    SDL_Quit();

    LOG_TRACE("Done destroying window.");
}

void Window::SetFullScreen(bool value)
{
    auto window = GetSDLWindowPtr(m_NativeWindowHandle);

    if (value)
    {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
        SDL_SetWindowFullscreen(window, 0);
    }
}

void Window::CenterMouse(bool drag)
{
    auto window = GetSDLWindowPtr(m_NativeWindowHandle);
    if (drag)
    {
        // XXX: Need proper rounding.
        SDL_WarpMouseInWindow(window, m_Width / 2.0f, m_Height / 2.0f);
        SDL_SetWindowGrab(window, SDL_TRUE);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    else
    {
        SDL_SetWindowGrab(window, SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
}

void Window::HandleEvents()
{
    SDL_Event event;
    bool doneHandlingEvents = false;

    while (SDL_PollEvent(&event))
    {
        // ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT: {
            m_ShouldClose = true;
            doneHandlingEvents = true;
            break;
        }

        // Handle subevents
        case SDL_WINDOWEVENT: {
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED: {
                u32 newWidth = (u32)(event.window.data1);
                u32 newHeight = (u32)(event.window.data2);

                // Update only if changed.
                if (newWidth != m_Width || newHeight != m_Height)
                {
                    m_Resized = true;
                    m_Width = newWidth;
                    m_Height = newHeight;

                    LOG_INFO("Window resized to ", m_Width, ", ", m_Height);
                }

                break;
            }

            case SDL_WINDOWEVENT_FOCUS_GAINED: {
                break;
            }
            case SDL_WINDOWEVENT_FOCUS_LOST: {
                break;
            }
            case SDL_WINDOWEVENT_MAXIMIZED: {
                m_Minimized = false;
                break;
            }
            case SDL_WINDOWEVENT_MINIMIZED: {
                m_Minimized = true;
                break;
            }
            case SDL_WINDOWEVENT_RESTORED: {
                m_Minimized = false;
                break;
            }
            case SDL_WINDOWEVENT_TAKE_FOCUS: {
                break;
            }
            case SDL_WINDOWEVENT_EXPOSED: {
                break;
            }

            case SDL_WINDOWEVENT_CLOSE: {
                m_ShouldClose = true;
                doneHandlingEvents = true;
                break;
            }
            default: {
                m_RefreshRate = GetRefreshRate();
                break;
            }
            }
            break;
        }
        }

        if (doneHandlingEvents)
        {
            break;
        }
    }
}