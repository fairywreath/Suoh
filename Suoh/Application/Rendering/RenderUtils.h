#pragma once

#include <RenderLib/Vulkan/VulkanDevice.h>

using namespace RenderLib;
using namespace RenderLib::Vulkan;

// Image +  sampler for shader binding.
struct Texture
{
    ImageHandle image;
    SamplerHandle sampler;
};

inline Texture MakeTexture(const ImageHandle& image, const SamplerHandle& sampler)
{
    Texture texture;
    texture.image = image;
    texture.sampler = sampler;
    texture.image->sampler = sampler->sampler;
    return texture;
}

inline ImageDesrciptorItem MakeFSImageDescriptor(ImageHandle image)
{
    ImageDesrciptorItem desc;
    desc.descriptorItem.type = vk::DescriptorType::eCombinedImageSampler;
    desc.descriptorItem.shaderStageFlags = vk::ShaderStageFlagBits::eFragment;
    desc.image = image;
    return desc;
}

inline BufferDescriptorItem MakeBufferDescriptor(BufferHandle buffer, vk::DescriptorType type,
                                                 vk::ShaderStageFlags shaderStageFlags, u32 size,
                                                 u32 offSet = 0)
{
    BufferDescriptorItem desc;
    desc.buffer = buffer;
    desc.offset = offSet;
    desc.size = size;
    desc.descriptorItem.shaderStageFlags = shaderStageFlags;
    desc.descriptorItem.type = type;

    return desc;
}

inline BufferDescriptorItem MakeVSFSUniformBufferDescriptor(BufferHandle buffer, u32 size,
                                                            u32 offSet = 0)
{
    return MakeBufferDescriptor(
        buffer, vk::DescriptorType::eUniformBuffer,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, size, offSet);
}

inline BufferDescriptorItem MakeVSUniformBufferDescriptor(BufferHandle buffer, u32 size,
                                                          u32 offSet = 0)
{
    return MakeBufferDescriptor(buffer, vk::DescriptorType::eUniformBuffer,
                                vk::ShaderStageFlagBits::eVertex, size, offSet);
}

inline BufferDescriptorItem MakeVSStorageBufferDescriptor(BufferHandle buffer, u32 size,
                                                          u32 offSet = 0)
{
    return MakeBufferDescriptor(buffer, vk::DescriptorType::eStorageBuffer,
                                vk::ShaderStageFlagBits::eVertex, size, offSet);
}

inline BufferDescriptorItem MakeFSStorageBufferDescriptor(BufferHandle buffer, u32 size,
                                                          u32 offSet = 0)
{
    return MakeBufferDescriptor(buffer, vk::DescriptorType::eStorageBuffer,
                                vk::ShaderStageFlagBits::eFragment, size, offSet);
}

inline BufferDescriptorItem MakeVSFSStorageBufferDescriptor(BufferHandle buffer, u32 size,
                                                            u32 offSet = 0)
{
    return MakeBufferDescriptor(
        buffer, vk::DescriptorType::eStorageBuffer,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, size, offSet);
}

// Bitmap to create cubemaps.
enum BitmapType
{
    BitmapType2D,
    BitmapTypeCube
};

enum BitmapFormat
{
    BitmapFormatUnsignedByte,
    BitmapFormatFloat,
};

struct Bitmap
{
    Bitmap() = default;
    Bitmap(int w, int h, int comp, BitmapFormat fmt)
        : w_(w), h_(h), comp_(comp), fmt_(fmt), data_(w * h * comp * GetBytesPerComponent(fmt))
    {
        InitGetSetFuncs();
    }
    Bitmap(int w, int h, int d, int comp, BitmapFormat fmt)
        : w_(w), h_(h), d_(d), comp_(comp), fmt_(fmt),
          data_(w * h * d * comp * GetBytesPerComponent(fmt))
    {
        InitGetSetFuncs();
    }
    Bitmap(int w, int h, int comp, BitmapFormat fmt, const void* ptr)
        : w_(w), h_(h), comp_(comp), fmt_(fmt), data_(w * h * comp * GetBytesPerComponent(fmt))
    {
        InitGetSetFuncs();
        memcpy(data_.data(), ptr, data_.size());
    }
    int w_ = 0;
    int h_ = 0;
    int d_ = 1;
    int comp_ = 3;
    BitmapFormat fmt_ = BitmapFormatUnsignedByte;
    BitmapType type_ = BitmapType2D;
    std::vector<u8> data_;

    static int GetBytesPerComponent(BitmapFormat fmt)
    {
        if (fmt == BitmapFormatUnsignedByte)
            return 1;
        if (fmt == BitmapFormatFloat)
            return 4;
        return 0;
    }

    void SetPixel(int x, int y, const glm::vec4& c)
    {
        (*this.*SetPixelFunc)(x, y, c);
    }
    glm::vec4 GetPixel(int x, int y) const
    {
        return ((*this.*GetPixelFunc)(x, y));
    }

private:
    using SetPixel_t = void (Bitmap::*)(int, int, const glm::vec4&);
    using GetPixel_t = glm::vec4 (Bitmap::*)(int, int) const;
    SetPixel_t SetPixelFunc = &Bitmap::SetPixelUnsignedByte;
    GetPixel_t GetPixelFunc = &Bitmap::GetPixelUnsignedByte;

    void InitGetSetFuncs()
    {
        switch (fmt_)
        {
        case BitmapFormatUnsignedByte:
            SetPixelFunc = &Bitmap::SetPixelUnsignedByte;
            GetPixelFunc = &Bitmap::GetPixelUnsignedByte;
            break;
        case BitmapFormatFloat:
            SetPixelFunc = &Bitmap::SetPixelFloat;
            GetPixelFunc = &Bitmap::GetPixelFloat;
            break;
        }
    }

    void SetPixelFloat(int x, int y, const glm::vec4& c)
    {
        const int ofs = comp_ * (y * w_ + x);
        float* data = reinterpret_cast<float*>(data_.data());
        if (comp_ > 0)
            data[ofs + 0] = c.x;
        if (comp_ > 1)
            data[ofs + 1] = c.y;
        if (comp_ > 2)
            data[ofs + 2] = c.z;
        if (comp_ > 3)
            data[ofs + 3] = c.w;
    }
    glm::vec4 GetPixelFloat(int x, int y) const
    {
        const int ofs = comp_ * (y * w_ + x);
        const float* data = reinterpret_cast<const float*>(data_.data());
        return glm::vec4(comp_ > 0 ? data[ofs + 0] : 0.0f, comp_ > 1 ? data[ofs + 1] : 0.0f,
                         comp_ > 2 ? data[ofs + 2] : 0.0f, comp_ > 3 ? data[ofs + 3] : 0.0f);
    }

    void SetPixelUnsignedByte(int x, int y, const glm::vec4& c)
    {
        const int ofs = comp_ * (y * w_ + x);
        if (comp_ > 0)
            data_[ofs + 0] = u8(c.x * 255.0f);
        if (comp_ > 1)
            data_[ofs + 1] = u8(c.y * 255.0f);
        if (comp_ > 2)
            data_[ofs + 2] = u8(c.z * 255.0f);
        if (comp_ > 3)
            data_[ofs + 3] = u8(c.w * 255.0f);
    }
    glm::vec4 GetPixelUnsignedByte(int x, int y) const
    {
        const int ofs = comp_ * (y * w_ + x);
        return glm::vec4(comp_ > 0 ? float(data_[ofs + 0]) / 255.0f : 0.0f,
                         comp_ > 1 ? float(data_[ofs + 1]) / 255.0f : 0.0f,
                         comp_ > 2 ? float(data_[ofs + 2]) / 255.0f : 0.0f,
                         comp_ > 3 ? float(data_[ofs + 3]) / 255.0f : 0.0f);
    }
};

Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& bitmap);
Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& bitmap);
