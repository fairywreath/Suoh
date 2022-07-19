#include "VKTextureHandler.h"
#include "../VKRenderDevice.h"

namespace Suou
{

struct Texture
{
    VkSampler textureSampler;
    VkImage image;
    VmaAllocation allocation;
};

}