#include "Application.h"

#include <Logger.h>
#include <PlatformDefines.h>

#if defined(SU_PLATFORM_WINDOWS)
#include <Windows/WindowsWindow.h>
#endif

#include <RenderLib/Vulkan/VKRenderDevice.h>

namespace Suoh
{

Application::Application()
    : mIsInitialized(false)
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

    // display window
    while (!mMainWindow->shouldClose())
    {
        // LOG_DEBUG("rendering... ");
        mRenderer.render();

        mMainWindow->update();
    }
}

void Application::init()
{
    mMainWindow.reset();

    WindowProperties props = {
        .width = 1280,
        .height = 720,
        .title = "Suoh Engine",
    };

#if defined(SU_PLATFORM_WINDOWS)
    mMainWindow = std::make_unique<WindowsWindow>(props);
#endif

    mRenderer.init(mMainWindow.get());

    mIsInitialized = true;
}

// void Application::setupGraphics()
// {
//     // test
//     BufferHandle buf = mRenderDevice->createBuffer({
//         .size = 64,
//         .usage = BufferUsage::UNIFORM_BUFFER,
//     });
//     LOG_DEBUG("BufferHandle: ", static_cast<type_safe::underlying_type<BufferHandle>>(buf));

//     BufferHandle buf2 = mRenderDevice->createBuffer({
//         .size = 32,
//         .usage = BufferUsage::VERTEX_BUFFER,
//     });
//     LOG_DEBUG("BufferHandle 2: ", static_cast<type_safe::underlying_type<BufferHandle>>(buf2));

//     mRenderDevice->destroyBuffer(buf2);
//     mRenderDevice->destroyBuffer(buf);

//     ShaderHandle shader = mRenderDevice->createShader({
//         .filePath = "Shaders/triangle_simple.vert",
//     });

//     mRenderDevice->destroyShader(shader);
// }

void Application::destroy()
{
    mMainWindow->destroy();
}

} // namespace Suoh
