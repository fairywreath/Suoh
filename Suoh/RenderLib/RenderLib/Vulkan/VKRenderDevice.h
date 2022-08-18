#pragma once

#include <vk_mem_alloc.h>

#include <SuohBase.h>

#include "Window.h"

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

/*
 * Temp. (monolithic) implementation of a vulkan render devoce
 */
class VKRenderDevice
{
public:
    explicit VKRenderDevice(Window* window);
    ~VKRenderDevice();

    Suoh_NON_COPYABLE(VKRenderDevice);
    Suoh_NON_MOVEABLE(VKRenderDevice);

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

    void destroyBuffer(Buffer& buffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void mapMemory(VmaAllocation allocation, void** data);
    void unmapMemory(VmaAllocation allocation);

    void uploadBufferData(Buffer& buffer, const void* data, const size_t dataSize);

    /*
     * Images and textures
     */
    bool createImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VmaMemoryUsage memUsage, Image& image, VkImageCreateFlags flags = 0);
    bool createImageView(VkFormat format, VkImageAspectFlags aspectFlags, Image& image);
    bool createTextureSampler(Texture& texture);
    bool createTextureImage(const std::string& filePath, Texture& texture);

    bool createTextureImageFromData(const void* data, u32 width, u32 height, VkFormat format, Image& image, u32 layerCount = 1, VkImageCreateFlags createFlags = 0);
    bool updateTextureImage(Image& image, u32 width, u32 height, VkFormat format, const void* imageData, u32 layerCount = 1, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

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
     * Misc./debug
     */
    bool setVkObjectName(void* object, VkObjectType objectType, const std::string& name);

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

    VkCommandPool mCommandPool;

    // temp command buffers, 1 per swapchain image
    std::vector<VkCommandBuffer> mCommandBuffers;

    // synchronization objects
    VkSemaphore mRenderSemaphore;
};

} // namespace Suoh
