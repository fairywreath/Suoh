#pragma once

#include "Camera.h"
#include "SceneRenderer.h"
#include "Window//Window.h"

class FpsCameraWindowObserver : public WindowObserver
{
public:
    explicit FpsCameraWindowObserver(Window& window);

    void OnCursorPos(double x, double y) override;
    void OnMouseButton(int key, int action, int mods) override;
    void OnKey(int key, int scancode, int action, int mods) override;
    void OnResize(int width, int height) override;
    void OnScroll(double xoffset, double yoffset) override;

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
    ~MainRenderer();

    NON_COPYABLE(MainRenderer);
    NON_MOVEABLE(MainRenderer);

    void Render();

    void Update(float deltaSeconds);

private:
    void Init();

private:
    Window& m_Window;

    std::unique_ptr<VulkanDevice> m_Device;
    std::unique_ptr<RenderDevice> m_RenderDevice;

    std::vector<CommandListHandle> m_CommandLists;

    std::unique_ptr<SceneRenderer> m_SceneRenderer;

    Texture m_EnvMapTexture;
    Texture m_IrrMapTexture;

    SceneData m_SceneData;

    FpsCameraWindowObserver mFpsCamWindowObserver;
    Camera mCamera;
};