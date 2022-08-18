#pragma once

#include <RenderLib/Vulkan/VKRenderDevice.h>
#include <SuohBase.h>

#include "Renderers/CanvasRenderer.h"
#include "Renderers/ClearRenderer.h"
#include "Renderers/FinishRenderer.h"
#include "Renderers/GuiRenderer.h"
#include "Renderers/ModelRenderer.h"

namespace Suoh
{

class MainRenderer
{
public:
    MainRenderer() = default;
    ~MainRenderer() = default;
    Suoh_NON_COPYABLE(MainRenderer);
    Suoh_NON_MOVEABLE(MainRenderer);

    void init(Window* window);
    void render();

private:
    struct ModelUniformBuffer
    {
        mat4 mvp;
    };

    void update();
    void composeFrame();

private:
    Window* mWindow{nullptr};
    std::unique_ptr<VKRenderDevice> mRenderDevice{nullptr};

    std::unique_ptr<ModelRenderer> mModelRenderer{nullptr};
    std::unique_ptr<ClearRenderer> mClearRenderer{nullptr};
    std::unique_ptr<FinishRenderer> mFinishRenderer{nullptr};
    std::unique_ptr<CanvasRenderer> mCanvasRenderer{nullptr};
    std::unique_ptr<GuiRenderer> mGuiRenderer{nullptr};

    std::vector<RendererBase*> mRenderers;
};

} // namespace Suoh