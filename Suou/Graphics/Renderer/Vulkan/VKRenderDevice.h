#pragma once

#include <vk_mem_alloc.h>

#include "Window.h"

#include "Renderer/RenderDevice.h"
#include "VKDevice.h"
#include "VKSwapchain.h"
#include "Handlers/VKBufferHandler.h"
#include "Handlers/VKUploadBufferHandler.h"
#include "Handlers/VKImageHandler.h"

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

    ImageHandle createImage(const ImageDescription& desc) override final;
    void destroyImage(ImageHandle) override final;

    void uploadToBuffer(BufferHandle dstBufferHandle, u64 dstOffset, const void* data, u64 srcOffset, u64 size) override final;

private:
    void init();
    void initCommands();
    void initAllocator();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    bool mInitialized;

    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkSurfaceKHR mSurface;

    VKDevice mDevice;
    VKSwapchain mSwapchain;

    VKBufferHandler mBufferHandler;
    VKUploadBufferHandler mUploadBufferHandler;
    VKImageHandler mImageHandler;

    VkCommandPool mCommandPool;

    VmaAllocator mAllocator;

    friend class VKBufferHandler;
    friend class VKUploadBufferHandler;
    friend class VKImageHandler;
};

}