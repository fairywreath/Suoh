#include "PBRModelRenderer.h"

#include <Core/Logger.h>

#include <gli/gli.hpp>
#include <gli/load_ktx.hpp>
#include <gli/texture2d.hpp>

namespace Suoh
{

static constexpr VkClearColorValue clearValueColor = {1.0f, 1.0f, 1.0f, 1.0f};

bool PBRModelRenderer::createDescriptors(u32 uniformDataSize)
{
    mRenderDevice->createDescriptorPool(1, 2, 8, mDescriptorPool);

    const std::vector<VkDescriptorSetLayoutBinding> bindings = {
        descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
        descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),

        descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // AO
        descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // Emissive
        descriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // Albedo
        descriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // MeR
        descriptorSetLayoutBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // Normal

        descriptorSetLayoutBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // env
        descriptorSetLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT), // env_IRR

        descriptorSetLayoutBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // brdfLUT
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    mRenderDevice->createDescriptorSetLayout(bindings, mDescriptorSetLayout);

    auto swapchainImagesCount = mRenderDevice->getSwapchainImageCount();
    std::vector<VkDescriptorSetLayout> layouts(swapchainImagesCount, mDescriptorSetLayout);

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = (u32)swapchainImagesCount,
        .pSetLayouts = layouts.data(),
    };

    mDescriptorSets.resize(swapchainImagesCount);
    mRenderDevice->allocateDescriptorSets(mDescriptorPool, layouts, mDescriptorSets);

    for (auto i = 0; i < swapchainImagesCount; i++)
    {
        VkDescriptorSet ds = mDescriptorSets[i];

        const VkDescriptorBufferInfo bufferInfo = {mUniformBuffers[i].buffer, 0, uniformDataSize};
        const VkDescriptorBufferInfo bufferInfo2 = {mStorageBuffer.buffer, 0, mVertexBufferSize};
        const VkDescriptorBufferInfo bufferInfo3 = {mStorageBuffer.buffer, mIndexBufferOffset, mIndexBufferSize};

        const VkDescriptorImageInfo imageInfoAO = {mTextureAO.sampler, mTextureAO.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoEmissive = {mTextureEmissive.sampler, mTextureEmissive.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoAlbedo = {mTextureAlbedo.sampler, mTextureAlbedo.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoMeR = {mTextureMeR.sampler, mTextureMeR.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoNormal = {mTextureNormal.sampler, mTextureNormal.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        const VkDescriptorImageInfo imageInfoEnv = {mEnvMap.sampler, mEnvMap.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoEnvIrr = {mEnvMapIrradiance.sampler, mEnvMapIrradiance.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo imageInfoBRDF = {mBrdfLUT.sampler, mBrdfLUT.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        const std::vector<VkWriteDescriptorSet> descriptorWrites = {
            bufferWriteDescriptorSet(ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
            imageWriteDescriptorSet(ds, &imageInfoAO, 3),
            imageWriteDescriptorSet(ds, &imageInfoEmissive, 4),
            imageWriteDescriptorSet(ds, &imageInfoAlbedo, 5),
            imageWriteDescriptorSet(ds, &imageInfoMeR, 6),
            imageWriteDescriptorSet(ds, &imageInfoNormal, 7),

            imageWriteDescriptorSet(ds, &imageInfoEnv, 8),
            imageWriteDescriptorSet(ds, &imageInfoEnvIrr, 9),
            imageWriteDescriptorSet(ds, &imageInfoBRDF, 10),
        };

        mRenderDevice->updateDescriptorSets((u32)descriptorWrites.size(), descriptorWrites);
    }

    return true;
}

void PBRModelRenderer::recordCommands(VkCommandBuffer commandBuffer)
{
    beginRenderPass(commandBuffer);

    vkCmdDraw(commandBuffer, static_cast<u32>(mIndexBufferSize / (sizeof(u32))), 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
}

void PBRModelRenderer::updateUniformBuffer(const void* data, size_t dataSize)
{
    auto currentImage = mRenderDevice->getSwapchainImageIndex();

    mRenderDevice->uploadBufferData(mUniformBuffers[currentImage], data, dataSize);
}

static void loadTexture(VKRenderDevice* renderDevice, const std::string& fileName, Texture& texture)
{
    renderDevice->createTextureImage(fileName, texture);
    renderDevice->createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, texture.image);
    renderDevice->createTextureSampler(texture);
}

static void loadCubeMap(VKRenderDevice* renderDevice, const std::string& fileName, Texture& texture, u32 mipLevels = 1)
{

    renderDevice->createCubeTextureImage(texture.image, fileName);
    renderDevice->createImageView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, texture.image, VK_IMAGE_VIEW_TYPE_CUBE, 6);
    renderDevice->createTextureSampler(texture);
}

PBRModelRenderer::PBRModelRenderer(VKRenderDevice* renderDevice,
                                   Window* window,
                                   Image depthImage,
                                   u32 uniformBufferSize,
                                   const std::string& modelFile,
                                   const std::string& texAOFile,
                                   const std::string& texEmissiveFile,
                                   const std::string& texAlbedoFile,
                                   const std::string& texMeRFile,
                                   const std::string& texNormalFile,
                                   const std::string& texEnvMapFile,
                                   const std::string& texIrrMapFile)
    : RendererBase(renderDevice, window, depthImage)
{
    // mDepthImage = depthImage;
    mRenderDevice->createDepthResources(mWindow->getWidth(), mWindow->getHeight(), mDepthImage);

    mRenderDevice->createPBRVertexBuffer(modelFile, mStorageBuffer, mVertexBufferSize, mIndexBufferSize, mIndexBufferOffset);

    loadTexture(mRenderDevice, texAOFile, mTextureAO);
    loadTexture(mRenderDevice, texEmissiveFile, mTextureEmissive);
    loadTexture(mRenderDevice, texAlbedoFile, mTextureAlbedo);
    loadTexture(mRenderDevice, texMeRFile, mTextureMeR);
    loadTexture(mRenderDevice, texNormalFile, mTextureNormal);

    // cube maps
    loadCubeMap(mRenderDevice, texEnvMapFile, mEnvMap);
    loadCubeMap(mRenderDevice, texIrrMapFile, mEnvMapIrradiance);

    gli::texture gliTex = gli::load_ktx("../../../Resources/brdfLUT.ktx");
    glm::tvec3<GLsizei> extent(gliTex.extent(0));

    mRenderDevice->createTextureImageFromData((u8*)gliTex.data(0, 0, 0), extent.x, extent.y, VK_FORMAT_R16G16_SFLOAT, mBrdfLUT.image);
    mRenderDevice->createImageView(VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, mBrdfLUT.image);
    mRenderDevice->createTextureSampler(mBrdfLUT, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    createUniformBuffers(uniformBufferSize);
    createDescriptors(uniformBufferSize);

    mRenderDevice->createColorDepthRenderPass(
        RenderPassCreateInfo{
            .clearColor = false,
            .clearDepth = false,
            .flags = 0,
        },
        true, mRenderPass);
    mRenderDevice->createPipelineLayout(mDescriptorSetLayout, mPipelineLayout);
    mRenderDevice->createGraphicsPipeline(mWindow->getWidth(), mWindow->getHeight(), mRenderPass, mPipelineLayout,
                                          std::vector<std::string>{
                                              "../../../Suoh/Shaders/PBR/PBR_Mesh.vert",
                                              "../../../Suoh/Shaders/PBR/PBR_Mesh.frag",
                                          },
                                          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                          mGraphicsPipeline);
    mRenderDevice->createColorDepthSwapchainFramebuffers(mRenderPass, mDepthImage.imageView, mSwapchainFramebuffers);
}

PBRModelRenderer::~PBRModelRenderer()
{
    mRenderDevice->destroyTexture(mTextureAO);
    mRenderDevice->destroyTexture(mTextureEmissive);
    mRenderDevice->destroyTexture(mTextureAlbedo);
    mRenderDevice->destroyTexture(mTextureMeR);
    mRenderDevice->destroyTexture(mTextureNormal);
    mRenderDevice->destroyTexture(mEnvMapIrradiance);
    mRenderDevice->destroyTexture(mEnvMap);
    mRenderDevice->destroyTexture(mBrdfLUT);

    mRenderDevice->destroyBuffer(mStorageBuffer);

    for (auto& frameBuffer : mSwapchainFramebuffers)
    {
        mRenderDevice->destroyFramebuffer(frameBuffer);
    }

    mRenderDevice->destroyRenderPass(mRenderPass);
    mRenderDevice->destroyPipelineLayout(mPipelineLayout);
    mRenderDevice->destroyPipeline(mGraphicsPipeline);

    mRenderDevice->destroyDescriptorSetLayout(mDescriptorSetLayout);
    mRenderDevice->destroyDescriptorPool(mDescriptorPool);

    mRenderDevice->destroyImage(mDepthImage);

    for (auto& buffer : mUniformBuffers)
    {
        mRenderDevice->destroyBuffer(buffer);
    }
}

} // namespace Suoh
