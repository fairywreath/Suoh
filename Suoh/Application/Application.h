#pragma once

#include "Rendering/MainRenderer.h"

#include "Window/Window.h"

class Application
{
public:
    Application();
    ~Application();

    NON_COPYABLE(Application);
    NON_MOVEABLE(Application);

    void Run();

private:
    Window m_Window;
    MainRenderer m_Renderer;
};