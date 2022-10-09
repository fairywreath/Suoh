#pragma once

#include <Core/SuohBase.h>
#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>
#include <Graphics/Scene/Scene.h>

#include "Renderers/CanvasRenderer.h"
#include "Renderers/ClearRenderer.h"
#include "Renderers/CubeRenderer.h"
#include "Renderers/FinishRenderer.h"
#include "Renderers/GuiRenderer.h"
#include "Renderers/ModelRenderer.h"
#include "Renderers/MultiMeshRenderer.h"
#include "Renderers/QuadRenderer.h"

#include "Renderers/PBRModelRenderer.h"

#include "MultiRenderer.h"

#include "Camera.h"
#include "FramesPerSecondCounter.h"

namespace Suoh
{

/*
 * XXX: this should ideally be placed outside MainRenderer
 * MainRenderer should only see the Camera interface
 */
class FpsCameraWindowObserver : public WindowObserver
{
public:
    explicit FpsCameraWindowObserver(Window& window);

    void onCursorPos(double x, double y) override;
    void onMouseButton(int key, int action, int mods) override;
    void onKey(int key, int scancode, int action, int mods) override;
    void onResize(int width, int height) override;
    void onScroll(double xoffset, double yoffset) override;

    FirstPersonCameraController& getCameraController();

    void update(float deltaSeconds);

private:
    struct MouseState
    {
        vec2 position{0.0f};
        bool pressedLeft{false};
    } mouseState;

private:
    Window& mWindow;
    FirstPersonCameraController mCameraController;
};

class MainRenderer
{
public:
    explicit MainRenderer(Window& window);

    ~MainRenderer() = default;
    SUOH_NON_COPYABLE(MainRenderer);
    SUOH_NON_MOVEABLE(MainRenderer);

    void init();
    void render(float deltaSeconds);

private:
    struct ModelUniformBuffer
    {
        mat4 mvp;
    };

    void update(float deltaSeconds);
    void composeFrame();

    void renderGui();

    void reinitCamera();

private:
    Window& mWindow;
    std::unique_ptr<VKRenderDevice> mRenderDevice{nullptr};

    // std::unique_ptr<ModelRenderer> mModelRenderer{nullptr};
    // std::unique_ptr<PBRModelRenderer> mPBRModelRenderer{nullptr};

     std::unique_ptr<ClearRenderer> mClearRenderer{nullptr};
     std::unique_ptr<FinishRenderer> mFinishRenderer{nullptr};
    // std::unique_ptr<CanvasRenderer> mCanvasRenderer{nullptr};
    // std::unique_ptr<GuiRenderer> mGuiRenderer{nullptr};
    // std::unique_ptr<CubeRenderer> mCubeRenderer{nullptr};

    // std::unique_ptr<MultiMeshRenderer> mMultiMeshRenderer{nullptr};

    // std::vector<RendererBase*> mRenderers;

    Texture envMap;
    Texture irrMap;

    SceneData sceneData1;
    std::unique_ptr<MultiRenderer> mMultiRenderer1;

    SceneData sceneData2;
    std::unique_ptr<MultiRenderer> mMultiRenderer2;

    FpsCameraWindowObserver mFpsCamWindowObserver;
    Camera mCamera;

    // XXX: sync fps and free moving camera positions and anglesfor seamless switching
    FreeMovingCameraController mFreeMovingCameraController;
    // XXX: yikes, can these be cached elsewhere?
    vec3 mFreeMovingCameraPos{0.0f};
    vec3 mFreeMovingCameraAngles{0.0f};

    FramesPerSecondCounter mFpsCounter;
};

} // namespace Suoh
