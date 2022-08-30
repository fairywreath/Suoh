#pragma once

#include <vk_mem_alloc.h>

#include <Core/SuohBase.h>

#include "Graphics/Window/Window.h"

#include "VKDevice.h"
#include "VKSwapchain.h"

namespace Suoh
{

/*
 * Temp. implementation of graphics resource types
 */
struct Buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Image
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation;
};

struct Texture
{
    Image image;
    VkSampler sampler;
};

using ShaderBinary = std::vector<u32>;

struct ShaderModule
{
    ShaderBinary SPIRV;
    size_t size;
    std::string binaryPath;

    VkShaderModule shaderModule;
};

struct GraphicsPipeline
{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

enum eRenderPassBit : u8
{
    eRenderPassBitFirst = 0x01,
    eRenderPassBitLast = 0x02,
    eRenderPassBitOffscreen = 0x04,
    eRenderPassBitOffscreenInternal = 0x08,
};

struct RenderPassCreateInfo
{
    bool clearColor = false;
    bool clearDepth = false;
    u8 flags = 0;
};

inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1)
{
    return VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = descriptorType,
        .descriptorCount = descriptorCount,
        .stageFlags = stageFlags,
        .pImmutableSamplers = nullptr,
    };
}

inline VkWriteDescriptorSet bufferWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorBufferInfo* bi, uint32_t bindIdx, VkDescriptorType dType)
{
    return VkWriteDescriptorSet{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        ds,
        bindIdx,
        0,
        1,
        dType,
        nullptr,
        bi,
        nullptr,
    };
}

inline VkWriteDescriptorSet imageWriteDescriptorSet(VkDescriptorSet ds, const VkDescriptorImageInfo* ii, uint32_t bindIdx)
{
    return VkWriteDescriptorSet{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        nullptr,
        ds,
        bindIdx,
        0,
        1,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        ii,
        nullptr,
        nullptr,
    };
}

/*
 * Temp. (monolithic) implementation of a vulkan render devoce
 */
class VKRenderDevice
{
public:
    explicit VKRenderDevice(Window* window);
    ~VKRenderDevice();

    SUOH_NON_COPYABLE(VKRenderDevice);
    SUOH_NON_MOVEABLE(VKRenderDevice);

    void destroy();

    /*
     * Misc. device operations
     */
    bool submit(const VkCommandBuffer& commandBuffer);
    bool present();

    bool deviceWaitIdle();

    // returns VkCommandBuffer that corresponds to the current swapchain image
    const VkCommandBuffer& getCurrentCommandBuffer();
    void resetCommandPool();

    /*
     * Swapchain operations
     */
    size_t getSwapchainImageIndex();
    size_t getSwapchainImageCount();
    void swapchainAcquireNextImage();

    /*
     *  Buffers
     */
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, Buffer& buffer);

    // loads 3d model, loads to vertex + indices SSBO
    bool createTexturedVertexBuffer(const std::string& filePath, Buffer& buffer, size_t& vertexBufferSize, size_t& indexBufferSize, size_t& indexBufferOffset);

    bool createPBRVertexBuffer(const std::string& filePath, Buffer& buffer, size_t& vertexBufferSize, size_t& indexBufferSize, size_t& indexBufferOffset);

    // shared buffer for compute
    bool createSharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memUsage, Buffer& buffer);

    void destroyBuffer(Buffer& buffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void mapMemory(VmaAllocation allocation, void** data);
    void unmapMemory(VmaAllocation allocation);

    void uploadBufferData(Buffer& buffer, const void* data, const size_t dataSize);
    void uploadBufferData(Buffer& buffer, const void* data, size_t offset, size_t dataSize);

    void downloadBuferData(Buffer& buffer, void* outData, size_t offset, size_t dataSize);

    /*
     * Images and textures
     */
    bool createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VmaMemoryUsage memUsage, Image& image, VkImageCreateFlags flags = 0);
    bool createImageView(VkFormat format, VkImageAspectFlags aspectFlags, Image& image, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, u32 layerCount = 1, u32 mipLevels = 1);
    bool createTextureSampler(Texture& texture, VkFilter minFilter = VK_FILTER_LINEAR, VkFilter magFilter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    bool createTextureImage(const std::string& filePath, Texture& texture);

    bool createTextureImageFromData(const void* data, u32 width, u32 height, VkFormat format, Image& image, u32 layerCount = 1, VkImageCreateFlags createFlags = 0);
    bool updateTextureImage(Image& image, u32 width, u32 height, VkFormat format, const void* imageData, u32 layerCount = 1, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

    bool createCubeTextureImage(Image& image, const std::string& filePath, u32* width = nullptr, u32* height = nullptr);

    void destroyImage(Image& image);
    void destroyTexture(Texture& texture);
    void copyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height, u32 layerCount = 1);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 layerCount = 1, u32 mipLevels = 1);
    void transitionImageLayourCmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
                                  VkImageLayout newLayout, u32 layerCount, u32 mipLevels);

    /*
     * Depth images
     */
    bool hasStencilComponent(VkFormat format);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    void createDepthResources(u32 width, u32 height, Image& depthImage);

    /*
     * Descriptor sets
     */
    bool createDescriptorPool(u32 uniformBufferCount, u32 storageBufferCount, u32 samplerCount, VkDescriptorPool& descriptorPool);
    bool createDescriptorPool(VkDescriptorPoolCreateInfo createInfo, VkDescriptorPool& descriptorPool);

    bool createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout& layout);

    bool allocateDescriptorSets(VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& layouts, std::vector<VkDescriptorSet>& descriptorSets);
    void updateDescriptorSets(u32 descriptorWriteCount, const std::vector<VkWriteDescriptorSet>& descriptorWrites);

    void destroyDescriptorSetLayout(VkDescriptorSetLayout descriptorLayout);
    void destroyDescriptorPool(VkDescriptorPool descriptorPool);

    /*
     * Shaders
     */
    bool createShader(const std::string& filePath, ShaderModule& shader);
    void destroyShader(ShaderModule& shader);

    /*
     * Graphics pipeline
     */
    bool createPipelineLayout(const VkDescriptorSetLayout& descriptorLayout, VkPipelineLayout& pipelineLayout);
    bool createPipelineLayoutWithConstants(const VkDescriptorSetLayout& descriptorLayout, VkPipelineLayout& pipelineLayout, u32 vertexConstSize, u32 fragConstSize);

    bool createColorDepthRenderPass(const RenderPassCreateInfo& createInfo, bool useDepth, VkRenderPass& renderPass, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);

    bool createGraphicsPipeline(u32 width, u32 height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
                                const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages, VkPrimitiveTopology topology, VkPipeline& pipeline,
                                bool useDepth = true, bool useBlending = true, bool dynamicScissorState = false);
    bool createGraphicsPipeline(u32 width, u32 height, VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
                                const std::vector<std::string>& shaderFilePaths, VkPrimitiveTopology topology, VkPipeline& pipeline,
                                bool useDepth = true, bool useBlending = true, bool dynamicScissorState = false);

    bool createColorDepthSwapchainFramebuffers(VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers);

    void destroyRenderPass(VkRenderPass renderPass);
    void destroyPipeline(VkPipeline pipeline);
    void destroyPipelineLayout(VkPipelineLayout pipelineLayout);
    void destroyFramebuffer(VkFramebuffer frameBuffer);

    /*
     * Compute pipeline
     */
    bool createComputePipeline(VkPipeline& pipeline, VkPipelineLayout pipelineLayout, VkShaderModule computeShader);
    bool executeComputeShader(VkPipeline computePipeline, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet,
                              u32 xsize, u32 ysize, u32 zsize);

    /*
     * Misc./debug
     */
    bool setVkObjectName(void* object, VkObjectType objectType, const std::string& name);

    size_t getMinStorageBufferOffset() const;

    inline VkDevice getVkDevice() const
    {
        return mDevice.getLogical();
    }

    /*
     * Misc. synchronization calls, used mainly for compute.
     * XXX: need to replace this with a dispatch/job system.
     */
    bool createFence(VkFence& fence, VkFenceCreateFlags flags = 0);
    void destroyFence(VkFence fence);

    void waitFence(VkFence fence);

    bool submitCompute(const VkCommandBuffer& commandBuffer, VkFence fence);
    const VkCommandBuffer& getComputeCommendBuffer() const
    {
        return mComputeCommandBuffer;
    }

private:
    void init();
    void initCommands();
    void initAllocator();
    void initSynchronizationObjects();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    bool mInitialized;

    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkSurfaceKHR mSurface;

    VKDevice mDevice;
    VKSwapchain mSwapchain;

    VmaAllocator mAllocator;

    VkSemaphore mRenderSemaphore;

    VkCommandPool mCommandPool;
    // temp command buffers, 1 per swapchain image
    std::vector<VkCommandBuffer> mCommandBuffers;

    // 1 command buffer for compute
    VkCommandPool mComputeCommandPool;
    VkCommandBuffer mComputeCommandBuffer;
};

} // namespace Suoh
