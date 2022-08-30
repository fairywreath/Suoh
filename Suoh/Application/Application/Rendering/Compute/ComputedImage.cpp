#include "ComputedImage.h"

namespace Suoh
{

ComputedImage::ComputedImage(VKRenderDevice* mRenderDevice, const std::string& shaderName, u32 textureWidth, u32 textureHeight, bool supportDownload)
    : ComputeItem(mRenderDevice, sizeof(u32)),
      ComputedWidth(textureWidth),
      ComputedHeight(textureHeight),
      mCanDownloadImage(supportDownload)
{
}

ComputedImage::~ComputedImage()
{
}

void ComputedImage::downloadImage(void* imageData)
{
}

bool ComputedImage::createComputedTexture(u32 computedWidth, u32 computedHeight, VkFormat format)
{
    return true;
}

bool ComputedImage::createDescriptorSet()
{
    return true;
}

bool ComputedImage::createComputeImageSetLayout()
{
    return true;
}

}; // namespace Suoh