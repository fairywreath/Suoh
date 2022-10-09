#include "Application.h"
#include "Profiler.h"

namespace Suoh
{

/*
 * imgui window input callbacks.
 */
void GuiWindowObserver::onCursorPos(double x, double y)
{
    //ImGui::GetIO().MousePos = ImVec2((float)x, (float)y);
}

void GuiWindowObserver::onMouseButton(int key, int action, int mods)
{
    //auto& io = ImGui::GetIO();
    //const int idx = (key == GLFW_MOUSE_BUTTON_LEFT) ? 0
    //                                                : ((key == GLFW_MOUSE_BUTTON_RIGHT) ? 2 : 1);
    //io.MouseDown[idx] = action == GLFW_PRESS;
}

void GuiWindowObserver::onKey(int key, int scancode, int action, int mods)
{
}

void GuiWindowObserver::onResize(int width, int height)
{
}

void GuiWindowObserver::onScroll(double xoffset, double yoffset)
{
}

/*
 * Main Application.
 */
Application::Application()
    : mIsInitialized(false),
      mMainWindow(WindowProperties{
          .width = 1420,
          .height = 720,
          .title = "Suoh Engine",
      }),
      mRenderer(mMainWindow)
{
    init();
}

Application::~Application()
{
    if (mIsInitialized)
    {
        destroy();
    }
}

void Application::run()
{
    if (!mIsInitialized)
    {
        return;
    }

    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    // display window
    while (!mMainWindow.shouldClose())
    {
        mRenderer.render(deltaSeconds);

        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        mMainWindow.update();
    }
}

void Application::init()
{
    mMainWindow.addObserver(mGuiWindowObserver);

    mIsInitialized = true;
}

void Application::destroy()
{
    mMainWindow.destroy();
}

} // namespace Suoh
