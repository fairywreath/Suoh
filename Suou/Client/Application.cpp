#include "Application.h"

#include <PlatformDefines.h>
#include <Logger.h>

#if defined(SU_PLATFORM_WINDOWS)
#include <Windows/WindowsWindow.h>
#endif

#include <Renderer/Vulkan/VKRenderDevice.h>

namespace Suou
{

Application::Application() :
    mIsInitialized(false)
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
    if (!mIsInitialized) return;

    // display window
    while(!mMainWindow->shouldClose())
    {
        mMainWindow->update();
    }
}

void Application::init()
{
    mMainWindow.reset();

    WindowProperties props{
        .width = 1280,
        .height = 720,
        .title = "Suou Engine"
    };

#if defined(SU_PLATFORM_WINDOWS)
    mMainWindow = std::make_unique<WindowsWindow>(props);
#endif

    mRenderDevice = std::make_unique<VKRenderDevice>();

    mIsInitialized = true;
}

void Application::destroy()
{
    mMainWindow->destroy();
}

}