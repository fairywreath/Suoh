#pragma once

#include <vk_mem_alloc.h>

#include "../VKCommon.h"
#include "../../Resources/UploadBuffer.h"
#include "../../Resources/Buffer.h"


namespace Suou
{

class VKRenderDevice;

struct IStagingBufferData {};

class VKUploadBufferHandler
{
public:
    explicit VKUploadBufferHandler(VKRenderDevice& renderDevice);
    ~VKUploadBufferHandler();

    void init();
    void destroy();

    void uploadToBuffer(BufferHandle dstBufferHandle, u64 dstOffset, const void* data, u64 srcOffset, u64 size);

private:
    void initStagingBuffers();
    void destroyStagingBuffers();

    // XXX: are these needed?
    UploadBuffer createUploadBuffer(BufferHandle targetBuffer, size_t size);
    void destroyUploadBuffer(UploadBuffer& uploadBuffer);

private:
    std::unique_ptr<IStagingBufferData> mData;
    VKRenderDevice& mRenderDevice;
};


}