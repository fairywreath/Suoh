#pragma once

#include <RenderLib/Vulkan/VulkanDevice.h>

using namespace RenderLib;
using namespace RenderLib::Vulkan;

/*
 * Light wrapper around API device.
 * XXX: Use a proper render graph system? Or make this class act like one with a compilation
 *      + execution stage.
 */
class RenderDevice
{
public:
    explicit RenderDevice(VulkanDevice* device);

    ImageHandle CreateTextureImage(const void* data, u32 width, u32 height, vk::Format format,
                                   u32 layerCount = 1,
                                   vk::ImageCreateFlags createFlags = vk::ImageCreateFlagBits{0},
                                   vk::ImageViewType = vk::ImageViewType::e2D);

    // Loads data from file and creates the image.
    ImageHandle CreateTextureImage(const std::string& fileName);
    ImageHandle CreateCubemapTextureImage(const std::string& fileName);
    ImageHandle CreateKTXTextureImage(const std::string& fileName);

    void UploadBufferData(BufferHandle buffer, const void* data, u32 size, u32 dstOffset = 0);
    void UploadImageData(ImageHandle image, const void* imageData);

    void TransitionImageLayout(Image* image, vk::ImageLayout layout);

    VulkanDevice* device;

private:
    CommandListHandle m_UploadCommandList;
};
