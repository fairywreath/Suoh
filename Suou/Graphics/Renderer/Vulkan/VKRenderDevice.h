#pragma once

#include "Renderer/RenderDevice.h"

#include "VKSwapchain.h"

namespace Suou
{

class VKRenderDevice : public RenderDevice
{

public:
    VKRenderDevice();
    ~VKRenderDevice() override;

    void destroy() override final;

private:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;

    bool mInitialized;
};

}