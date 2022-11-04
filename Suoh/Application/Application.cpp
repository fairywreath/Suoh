#include "Application.h"

static constexpr auto FRAMEBUFFER_WIDTH = 1420;
static constexpr auto FRAMEBUFFER_HEIGHT = 720;

Application::Application()
    : m_Window({
        .width = FRAMEBUFFER_WIDTH,
        .height = FRAMEBUFFER_HEIGHT,
        .title = "Suoh Engine",

    }),
      m_Renderer(m_Window)
{
}

Application::~Application()
{
}

void Application::Run()
{
    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    while (!m_Window.ShouldClose())
    {
        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        m_Renderer.Render();

        m_Window.Update();
        m_Renderer.Update(deltaSeconds);
    }
}
