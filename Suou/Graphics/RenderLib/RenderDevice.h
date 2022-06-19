#pragma once

#include <SuouBase.h>

#include "Resources/Buffer.h"
#include "Resources/GraphicsPipeline.h"
#include "Resources/Image.h"
#include "Resources/Sampler.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Resources/UploadBuffer.h"

namespace Suou
{

class RenderDevice
{
public:
    virtual ~RenderDevice(){};

    virtual void destroy() = 0;

    /*
     * Graphics resource management
     */
    virtual BufferHandle createBuffer(const BufferDescription& desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;

    virtual ImageHandle createImage(const ImageDescription& desc) = 0;
    virtual void destroyImage(ImageHandle handle) = 0;

    virtual void uploadToBuffer(BufferHandle dstBufferHandle, u64 dstOffset, const void* data, u64 srcOffset, u64 size)
        = 0;

    virtual GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDescription& desc) = 0;
    virtual void destroyGraphicsPipeline(GraphicsPipelineHandle handle) = 0;

    virtual ShaderHandle createShader(const ShaderDescription& desc) = 0;
    virtual void destroyShader(ShaderHandle handle) = 0;

    // virtual UploadBuffer createUploadBuffer(BufferHandle targetBuffer, size_t size) = 0;
    // virtual void destroyUploadBuffer(UploadBuffer uploadBuffer) = 0;

    // [[nodiscard]] virtual void* mapBuffer(BufferHandle handle) = 0;
    // virtual void unmapBuffer(BufferHandle handle) = 0;

protected:
    RenderDevice(){};
};

} // namespace Suou
