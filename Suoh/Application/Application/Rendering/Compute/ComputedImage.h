#pragma once

#include "ComputeItem.h"

namespace Suoh
{

class ComputedImage : public ComputeItem
{
public:
    ComputedImage(VKRenderDevice* mRenderDevice, const std::string& shaderName, u32 textureWidth, u32 textureHeight, bool supportDownload = false);
    ~ComputedImage() override;

    void downloadImage(void* imageData);

    Texture ComputedTexture;
    u32 ComputedWidth;
    u32 ComputedHeight;

protected:
    bool mCanDownloadImage;

    bool createComputedTexture(u32 computedWidth, u32 computedHeight, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    bool createDescriptorSet();
    bool createComputeImageSetLayout();
};

} // namespace Suoh