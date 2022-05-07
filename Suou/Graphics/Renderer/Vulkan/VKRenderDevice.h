#pragma once

#include <vk_mem_alloc.h>

#include "Window.h"

#include "Renderer/RenderDevice.h"
#include "VKDevice.h"
#include "VKSwapchain.h"
#include "Handlers/VKBufferHandler.h"

namespace Suou
{

class VKRenderDevice : public RenderDevice
{
public:
    explicit VKRenderDevice(Window* window);
    ~VKRenderDevice() override;

    void destroy() override final;
    
    BufferHandle createBuffer(const BufferDescription& desc) override final;

    void destroyBuffer(BufferHandle handle) override final;

private:
    void init();
    void initCommands();
    void initAllocator();

private:
    bool mInitialized;

    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkSurfaceKHR mSurface;

    VKDevice mDevice;
    VKSwapchain mSwapchain;

    VKBufferHandler mBufferHandler;

    VkCommandPool mCommandPool;

    VmaAllocator mAllocator;

    friend class VKBufferHandler;
};

}