#pragma once

#include <vk_mem_alloc.h>

#include "Window.h"

#include "../RenderDevice.h"
#include "VKDevice.h"
#include "VKSwapchain.h"

#include "Handlers/VKBufferHandler.h"
#include "Handlers/VKImageHandler.h"
#include "Handlers/VKPipelineHandler.h"
#include "Handlers/VKShaderHandler.h"
#include "Handlers/VKUploadBufferHandler.h"

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

    GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDescription& desc) override final;
    void destroyGraphicsPipeline(GraphicsPipelineHandle handle) override final;

    ShaderHandle createShader(const ShaderDescription& desc) override final;
    void destroyShader(ShaderHandle handle) override final;

    void uploadToBuffer(BufferHandle dstBufferHandle, u64 dstOffset, const void* data, u64 srcOffset,
                        u64 size) override final;

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
    VKShaderHandler mShaderHandler;
    VKPipelineHandler mPipelineHandler;

    VkCommandPool mCommandPool;

    VmaAllocator mAllocator;

    friend class VKBufferHandler;
    friend class VKUploadBufferHandler;
    friend class VKImageHandler;
    friend class VKPipelineHandler;
    friend class VKShaderHandler;
};

} // namespace Suou
