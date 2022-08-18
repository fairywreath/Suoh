#pragma once

#include <memory>

#include <SuohBase.h>
#include <Window.h>

#include <RenderLib/Vulkan/VKRenderDevice.h>

#include "Rendering/MainRenderer.h"

namespace Suoh
{

class Application
{
public:
    Application();
    ~Application();

    Suoh_NON_COPYABLE(Application);
    Suoh_NON_MOVEABLE(Application);

    void run();

    void init();
    void destroy();

private:
    bool mIsInitialized;

    std::unique_ptr<Window> mMainWindow;

    MainRenderer mRenderer;
};

} // namespace Suoh
