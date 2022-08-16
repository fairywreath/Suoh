#pragma once

#include <RenderLib/Vulkan/VKRenderDevice.h>
#include <SuouBase.h>

#include "ClearRenderer.h"
#include "FinishRenderer.h"
#include "ModelRenderer.h"

namespace Suou
{

class MainRenderer
{
public:
    MainRenderer(){};

    ~MainRenderer() = default;
    SUOU_NON_COPYABLE(MainRenderer);
    SUOU_NON_MOVEABLE(MainRenderer);

    void init(Window* window);

    void render();

private:
    struct UniformBuffer
    {
        mat4 mvp;
    } ubo;

private:
    void update();
    void composeFrame();

private:
    Window* mWindow{nullptr};
    std::unique_ptr<VKRenderDevice> mRenderDevice{nullptr};

    std::unique_ptr<ModelRenderer> mModelRenderer{nullptr};
    std::unique_ptr<ClearRenderer> mClearRenderer{nullptr};
    std::unique_ptr<FinishRenderer> mFinishRenderer{nullptr};

    std::vector<RendererBase*> mRenderers;
};

} // namespace Suou