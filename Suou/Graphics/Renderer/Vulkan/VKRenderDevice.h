#pragma once

#include "Renderer/RenderDevice.h"
#include "Window.h"

#include "VKDevice.h"
#include "VKSwapchain.h"

namespace Suou
{

class VKRenderDevice : public RenderDevice
{
public:
    explicit VKRenderDevice(Window* window);
    ~VKRenderDevice() override;

    void destroy() override final;


private:
    void init();
    void initCommands();

private:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkSurfaceKHR mSurface;

    VKDevice mDevice;
    VKSwapchain mSwapchain;

    VkCommandPool mCommandPool;

    bool mInitialized;
};

}