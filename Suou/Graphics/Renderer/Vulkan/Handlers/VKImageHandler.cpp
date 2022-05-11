#include "VKImageHandler.h"
#include "../VKRenderDevice.h"

#include <queue>

namespace Suou
{

struct Image
{
    VkImage image;
    VmaAllocation allocation;
};

struct VKImageHandlerData : IVKImageHandlerData
{
    std::vector<Image> images;
    std::queue<ImageHandle> returnedImageHandles;
};

static inline VKImageHandlerData& toImageHandlerData(IVKImageHandlerData* data)
{
    return static_cast<VKImageHandlerData&>(*data);
}

VKImageHandler::VKImageHandler(VKRenderDevice& renderDevice)
    : mRenderDevice(renderDevice), mData(std::make_unique<VKImageHandlerData>())
{
}

VKImageHandler::~VKImageHandler()
{
}

ImageHandle VKImageHandler::createImage(const ImageDescription& desc)
{
    auto device = mRenderDevice.mDevice.getLogical();
    auto& data = toImageHandlerData(mData.get());

    ImageHandle handle = acquireNewHandle();
    Image& image = data.images[toHandleType(handle)];

    // const VkImageCreateInfo imageCreateInfo =
    // {
    // 	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    // 	.pNext = nullptr,
    // 	.flags = flags,
    // 	.imageType = VK_IMAGE_TYPE_2D,
    // 	.format = format,
    // 	.extent = VkExtent3D {.width = width, .height = height, .depth = 1 },
    // 	.mipLevels = mipLevels,
    // 	.arrayLayers = (u32)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
    // 	.samples = VK_SAMPLE_COUNT_1_BIT,
    // 	.tiling = tiling,
    // 	.usage = usage,
    // 	.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    // 	.queueFamilyIndexCount = 0,
    // 	.pQueueFamilyIndices = nullptr,
    // 	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    // };

    return handle;
}

void VKImageHandler::destroyImage(ImageHandle image)
{
}

ImageHandle VKImageHandler::acquireNewHandle()
{
    ImageHandle handle;
    auto& data = toImageHandlerData(mData.get());

    if (!data.returnedImageHandles.empty())
    {
        handle = data.returnedImageHandles.front();
        data.returnedImageHandles.pop();
    }
    else
    {
        handle = ImageHandle(static_cast<HandleType>(data.images.size()));
        data.images.emplace_back();
    }

    return handle;
}

} // namespace Suou
