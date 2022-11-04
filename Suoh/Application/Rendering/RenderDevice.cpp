#include "RenderDevice.h"

#include "Rendering/RenderUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <gli/gli.hpp>
#include <gli/load_ktx.hpp>
#include <gli/texture2d.hpp>

#include <Logger.h>

#include <filesystem>

namespace fs = std::filesystem;

RenderDevice::RenderDevice(VulkanDevice* device) : device(device)
{
    CommandListDesc uploadListDesc;
    uploadListDesc.usage = CommandListUsage::TRANSFER;

    m_UploadCommandList = device->CreateCommandList(uploadListDesc);
}

ImageHandle RenderDevice::CreateTextureImage(const void* data, u32 width, u32 height,
                                             vk::Format format, u32 layerCount,
                                             vk::ImageCreateFlags createFlags,
                                             vk::ImageViewType viewType)
{
    ImageDesc imageDesc;
    imageDesc.width = width;
    imageDesc.height = height;
    imageDesc.format = format;
    imageDesc.usage = vk::ImageUsageFlagBits::eSampled;
    imageDesc.tiling = vk::ImageTiling::eOptimal;
    imageDesc.flags = createFlags;

    // ImageView desc.
    imageDesc.layerCount = layerCount;
    imageDesc.viewType = viewType;

    auto image = device->CreateImage(imageDesc);

    UploadImageData(image, data);

    image->CreateSubresourceView();

    return image;
}

ImageHandle RenderDevice::CreateTextureImage(const std::string& fileName)
{
    // Load the image.
    int texWidth;
    int texHeight;
    int texChannels;

    stbi_uc* pixels
        = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        LOG_ERROR("Failed to load texture: ", fs::absolute(fileName));
        return nullptr;
    }

    auto image = CreateTextureImage(pixels, texWidth, texHeight, vk::Format::eR8G8B8A8Unorm);

    stbi_image_free(pixels);

    return image;
}

inline void Float24to32(int w, int h, const float* img24, float* img32)
{
    const int numPixels = w * h;
    for (int i = 0; i != numPixels; i++)
    {
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = *img24++;
        *img32++ = 1.0f;
    }
}

ImageHandle RenderDevice::CreateCubemapTextureImage(const std::string& fileName)
{
    // Load the image.
    int w, h, comp;
    const float* img = stbi_loadf(fileName.c_str(), &w, &h, &comp, 3);
    std::vector<float> img32(w * h * 4);

    Float24to32(w, h, img, img32.data());
    if (!img)
    {
        LOG_ERROR("Failed to load image: ", fileName, "!");
        return nullptr;
    }
    stbi_image_free((void*)img);

    // Create cube bitmap.
    Bitmap in(w, h, 4, BitmapFormatFloat, img32.data());
    Bitmap out = ConvertEquirectangularMapToVerticalCross(in);
    Bitmap cube = ConvertVerticalCrossToCubeMapFaces(out);

    return CreateTextureImage(cube.data_.data(), cube.w_, cube.h_, vk::Format::eR32G32B32A32Sfloat,
                              6, vk::ImageCreateFlagBits::eCubeCompatible,
                              vk::ImageViewType::eCube);
}

ImageHandle RenderDevice::CreateKTXTextureImage(const std::string& fileName)
{
    gli::texture gliTex = gli::load_ktx(fileName);
    glm::tvec3<u32> extent(gliTex.extent(0));

    auto image = CreateTextureImage((u8*)gliTex.data(0, 0, 0), extent.x, extent.y,
                                    vk::Format::eR16G16Sfloat);

    return image;
}

void RenderDevice::UploadBufferData(BufferHandle buffer, const void* data, u32 size, u32 dstOffset)
{
    m_UploadCommandList->Begin();
    m_UploadCommandList->WriteBuffer(buffer, data, size, dstOffset);
    m_UploadCommandList->End();
    device->Submit(m_UploadCommandList);
    device->WaitGraphicsSubmissionSemaphore();
}

void RenderDevice::UploadImageData(ImageHandle image, const void* imageData)
{
    m_UploadCommandList->Begin();
    m_UploadCommandList->WriteImage(image, imageData);
    m_UploadCommandList->End();
    device->Submit(m_UploadCommandList);
    device->WaitGraphicsSubmissionSemaphore();
}

void RenderDevice::TransitionImageLayout(Image* image, vk::ImageLayout layout)
{
    m_UploadCommandList->Begin();
    m_UploadCommandList->TransitionImageLayout(image, layout);
    m_UploadCommandList->End();
    device->Submit(m_UploadCommandList);
    device->WaitGraphicsSubmissionSemaphore();
}
