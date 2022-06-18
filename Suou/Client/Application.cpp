#include "Application.h"

#include <Logger.h>
#include <PlatformDefines.h>

#if defined(SU_PLATFORM_WINDOWS)
#include <Windows/WindowsWindow.h>
#endif

#include <RenderLib/Vulkan/VKRenderDevice.h>

namespace Suou
{

Application::Application() : mIsInitialized(false)
{
    init();

    // test
    BufferHandle buf = mRenderDevice->createBuffer({
        .size = 64,
        .usage = BufferUsage::UNIFORM_BUFFER,
    });
    LOG_DEBUG("BufferHandle: ", static_cast<type_safe::underlying_type<BufferHandle>>(buf));
    mRenderDevice->destroyBuffer(buf);

    BufferHandle buf2 = mRenderDevice->createBuffer({
        .size = 32,
        .usage = BufferUsage::VERTEX_BUFFER,
    });
    LOG_DEBUG("BufferHandle 2: ", static_cast<type_safe::underlying_type<BufferHandle>>(buf2));
    mRenderDevice->destroyBuffer(buf2);
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

    // display window
    while (!mMainWindow->shouldClose())
    {
        mMainWindow->update();
    }
}

void Application::init()
{
    mMainWindow.reset();

    WindowProperties props = {
        .width = 1280,
        .height = 720,
        .title = "Suou Engine",
    };

#if defined(SU_PLATFORM_WINDOWS)
    mMainWindow = std::make_unique<WindowsWindow>(props);
#endif

    mRenderDevice = std::make_unique<VKRenderDevice>(mMainWindow.get());

    mIsInitialized = true;
}

void Application::destroy()
{
    mMainWindow->destroy();
}

} // namespace Suou
