#pragma once

#include <memory>

#include <Core/SuohBase.h>
#include <Graphics/Window/Window.h>

#include "Rendering/Camera.h"
#include "Rendering/MainRenderer.h"

namespace Suoh
{

class GuiWindowObserver : public WindowObserver
{
public:
    void onCursorPos(double x, double y) override;
    void onMouseButton(int key, int action, int mods) override;
    void onKey(int key, int scancode, int action, int mods) override;
    void onResize(int width, int height) override;
    void onScroll(double xoffset, double yoffset) override;
};

class Application
{
public:
    Application();
    ~Application();

    SUOH_NON_COPYABLE(Application);
    SUOH_NON_MOVEABLE(Application);

    void run();

    void init();
    void destroy();

private:
    bool mIsInitialized;

    Window mMainWindow;

    MainRenderer mRenderer;

    GuiWindowObserver mGuiWindowObserver;
};

} // namespace Suoh
