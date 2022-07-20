#pragma once

#include <memory>

#include <SuouBase.h>
#include <Window.h>

#include <RenderLib/Vulkan/VKRenderDevice.h>

#include "Rendering/MainRenderer.h"

namespace Suou
{

class Application
{
public:
    Application();
    ~Application();

    SUOU_NON_COPYABLE(Application);
    SUOU_NON_MOVEABLE(Application);

    void run();

    void init();
    void destroy();

private:
    bool mIsInitialized;

    std::unique_ptr<Window> mMainWindow;

    MainRenderer mRenderer;
};

} // namespace Suou
