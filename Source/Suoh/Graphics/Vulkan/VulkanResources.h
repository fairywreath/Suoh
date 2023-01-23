#pragma once

#include <variant>

#include "Graphics/GPUStates.h"

#include "VulkanCommon.h"
/*
 * GPU resource creation descriptors and implementation.
 */
class Sampler : public GPUResource
{
public:
    Sampler(VulkanDevice* device, const SamplerDesc& createDesc);
    virtual ~Sampler();

    const SamplerDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Sampler GetVulkanHandle() const
    {
        return m_VulkanSampler;
    }

private:
    VulkanDevice* m_Device;
    SamplerDesc m_Desc;

    vk::Sampler m_VulkanSampler;

    vk::Filter m_MinFilter{vk::Filter::eLinear};
    vk::Filter m_MagFilter{vk::Filter::eLinear};

    vk::SamplerMipmapMode m_MipFilter{vk::SamplerMipmapMode::eLinear};

    vk::SamplerAddressMode m_AddressModeU{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode m_AddressModeV{vk::SamplerAddressMode::eRepeat};
    vk::SamplerAddressMode m_AddressModeW{vk::SamplerAddressMode::eRepeat};

    vk::SamplerReductionMode m_ReductionMode{vk::SamplerReductionMode::eWeightedAverage};

    std::string m_Name;
};

class Texture : public GPUResource
{
public:
    Texture(VulkanDevice* device, const TextureDesc& desc);
    Texture(VulkanDevice* device, const TextureViewDesc& textureViewDesc);

    virtual ~Texture();

    void InitTextureView(const TextureViewDesc& desc);

    const TextureDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Image GetVulkanHandle() const
    {
        return m_VulkanImage;
    }

    vk::ImageView GetVulkanView() const
    {
        return m_VulkanImageView;
    }

    void SetResourceState(ResourceState state)
    {
        m_ResourceState = state;
    }

    u32 GetWidth() const
    {
        return m_Width;
    }

    u32 GetHeight() const
    {
        return m_Height;
    }

    u32 GetDepth() const
    {
        return m_Depth;
    }

    u32 GetMipLevelCount() const
    {
        return m_MipLevelCount;
    }

    u32 GetArrayLayerCount() const
    {
        return m_ArrayLayerCount;
    }

    VmaAllocation GetVmaAllocation() const
    {
        return m_VmaAllocation;
    }

    vk::Format GetFormat() const
    {
        return m_VulkanFormat;
    }

    TextureType GetType() const
    {
        return m_Type;
    }

    void LinkSampler(Sampler* sampler)
    {
        m_Sampler = sampler;
    }

    Sampler* GetSampler() const
    {
        return m_Sampler;
    }

private:
    // Creates a texture view of an already existing texture.
    void InitTextureViewFromExistingTexture(const TextureViewDesc& textureViewDesc);

private:
    VulkanDevice* m_Device;
    TextureDesc m_Desc;

    vk::Image m_VulkanImage;
    vk::ImageView m_VulkanImageView;
    vk::Format m_VulkanFormat;
    VmaAllocation m_VmaAllocation;

    ResourceState m_ResourceState{ResourceState::Undefined};

    u32 m_Width{1};
    u32 m_Height{1};
    u32 m_Depth{1};

    u32 m_ArrayLayerCount{1};
    u32 m_ArrayBaseLayer{0};
    u32 m_MipLevelCount{1};
    u32 m_MipBaseLevel{0};

    TextureFlags m_Flags;

    // XXX: Is a strong reference needed here?
    TextureRef m_ParentTexture;

    TextureType m_Type{TextureType::Texture2D};

    Sampler* m_Sampler{nullptr};
    // SamplerRef m_Sampler{nullptr};

    // vk::ImageLayout m_CurrentLayout;

    std::string m_Name;

    friend class VulkanDevice;
};
;

struct RenderPassDesc
{
    std::vector<RenderPassColorTarget> colorTargets;

    vk::Format depthStencilFormat{vk::Format::eUndefined};
    vk::ImageLayout depthStencilFinalLayout;

    RenderPassOperation depthOperation{RenderPassOperation::DontCare};
    RenderPassOperation stencilOperation{RenderPassOperation::DontCare};

    std::string name;

    RenderPassOutput& Reset();
    RenderPassOutput& AddColorTarget(vk::Format format, vk::ImageLayout layout, RenderPassOperation loadOp);
    RenderPassOutput& SetDepthTarget(vk::Format format, vk::ImageLayout layout);
    RenderPassOutput& SetDepthStencilOperations(RenderPassOperation depthOp, RenderPassOperation stencilOp);
};

class RenderPass : public GPUResource
{
public:
    RenderPass(VulkanDevice* device, const RenderPassDesc& desc);
    virtual ~RenderPass();

    const RenderPassDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::RenderPass GetVulkanHandle() const
    {
        return m_VulkanRenderPass;
    }

    const RenderPassOutput& GetOutput() const
    {
        return m_Output;
    }

    u32 NumColorRenderTargets() const
    {
        return m_NumColorRenderTargets;
    }

private:
    VulkanDevice* m_Device;
    RenderPassDesc m_Desc;

    vk::RenderPass m_VulkanRenderPass;

    RenderPassOutput m_Output;
    u32 m_NumColorRenderTargets{0};

    std::string m_Name;
};

struct FramebufferDesc
{
    // XXX: Need weak_ptr for these gpu resources?
    RenderPass* renderPass{nullptr};
    std::vector<Texture*> colorAttachments;
    Texture* depthStencilAttachment{nullptr};

    u32 width{0};
    u32 height{0};

    f32 scaleX{1.0f};
    f32 scaleY{1.0f};
    bool resize{false};

    std::string name;
};

class Framebuffer : public GPUResource
{
public:
    Framebuffer(VulkanDevice* device, const FramebufferDesc& desc);
    virtual ~Framebuffer();

    const FramebufferDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Framebuffer GetVulkanHandle() const
    {
        return m_VulkanFramebuffer;
    }

    u32 NumColorAttachments() const
    {
        return m_ColorAttachments.size();
    }

    Texture* GetColorAttachment(u32 index) const
    {
        assert(index < m_ColorAttachments.size());
        return m_ColorAttachments[index];
    }

    bool ContainsDepthStencilAttachment() const
    {
        return (m_DepthStencilAttachment != nullptr);
    }

    Texture* GetDepthStenciAttachment() const
    {
        return m_DepthStencilAttachment;
    }

    u32 GetWidth() const
    {
        return m_Width;
    }

    u32 GetHeight() const
    {
        return m_Height;
    }

private:
    VulkanDevice* m_Device;
    FramebufferDesc m_Desc;

    vk::Framebuffer m_VulkanFramebuffer;

    RenderPassRef m_RenderPass;
    std::vector<TextureRef> m_ColorAttachments;
    TextureRef m_DepthStencilAttachment;

    u32 m_Width{0};
    u32 m_Height{0};

    f32 m_ScaleX{1.0f};
    f32 m_ScaleY{1.0f};

    bool m_Resize{false};

    std::string m_Name;
};

struct PipelineDesc
{
    VertexInputDesc vertexInput{};
    BlendStateDesc blendState{};
    RasterizationState rasterizationState{};
    DepthStencilState depthStencilState{};

    u32 vertexConstSize{0};
    u32 fragmentConstSize{0};

    // XXX: Ideally we wont need these(and have creation descriptors of them instead), in case dynamic rendering is
    // used.
    ShaderState* shaderState;
    RenderPass* renderPass;

    std::vector<DescriptorSetLayout*> descriptorSetLayouts;

    // RenderPassOutput renderPassOutput;

    vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
    const ViewportState* viewport{nullptr};

    // Viewport state dimensions.
    u32 width{1};
    u32 height{1};

    // XXX: Params for dynamic states?

    std::string name;
    std::string pipelineCachePath;

    PipelineType type;

    PipelineDesc& AddDescriptorSetLayout(DescriptorSetLayout* descriptorSetLayout);
    PipelineDesc& SetName(const std::string& _name);
};

class Pipeline : public GPUResource
{
public:
    Pipeline(VulkanDevice* device, const PipelineDesc& desc);
    virtual ~Pipeline();

    const PipelineDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Pipeline GetVulkanHandle() const
    {
        return m_VulkanPipeline;
    }

    vk::PipelineLayout GetVulkanLayout() const
    {
        return m_VulkanPipelineLayout;
    }

    vk::PipelineBindPoint GetVulkanBindPoint() const
    {
        return m_VulkanPipelineBindPoint;
    }

private:
    void InitGraphicsPipeline(const PipelineDesc& desc, vk::PipelineCache pipelineCache = nullptr);
    void InitComputePipeline(const PipelineDesc& desc, vk::PipelineCache pipelineCache = nullptr);

private:
    VulkanDevice* m_Device;
    PipelineDesc m_Desc;

    vk::Pipeline m_VulkanPipeline;
    vk::PipelineLayout m_VulkanPipelineLayout;
    vk::PipelineBindPoint m_VulkanPipelineBindPoint;

    ShaderStateRef m_ShaderState;
    std::vector<DescriptorSetLayoutRef> m_DescriptorSetLayouts;

    DepthStencilState m_DepthStencilState;
    BlendStateDesc m_BlendState;
    RasterizationState m_RasterizationState;
    // ViewportState m_ViewportState;

    PipelineType m_Type;

    std::string m_Name;
};

/*
 * Misc. helpers.
 */
inline bool IsDepthStencil(vk::Format value)
{
    return value == vk::Format::eD16UnormS8Uint || value == vk::Format::eD24UnormS8Uint
           || value == vk::Format::eD32SfloatS8Uint;
}
inline bool IsDepthOnly(vk::Format value)
{
    return value >= vk::Format::eD16Unorm && value < vk::Format::eD32Sfloat;
}
inline bool IsStencilOnly(vk::Format value)
{
    return value == vk::Format::eS8Uint;
}
inline bool HasDepth(vk::Format value)
{
    return (value >= vk::Format::eD16Unorm && value < vk::Format::eS8Uint)
           || (value >= vk::Format::eD16UnormS8Uint && value <= vk::Format::eD32SfloatS8Uint);
}
inline bool HasStencil(vk::Format value)
{
    return value >= vk::Format::eS8Uint && value <= vk::Format::eD32SfloatS8Uint;
}
inline bool HasDepthOrStencil(vk::Format value)
{
    return value >= vk::Format::eD16Unorm && value <= vk::Format::eD32SfloatS8Uint;
}

/*
 * Render state type to vulkan type convertors.
 */
vk::Format ToVkFormatVertex(VertexComponentFormat value);
vk::ImageType ToVkImageType(TextureType type);
vk::ImageViewType ToVkImageViewType(TextureType type);
vk::PipelineStageFlags ToVkPipelineStageFlags(PipelineStage stage);
vk::PrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology value);

/*
 * Vulkan type to shader compilation argument convertors
 */
std::string ToShaderStageDefine(vk::ShaderStageFlagBits flagBits);
std::string ToShaderCompilerExtension(vk::ShaderStageFlagBits flagBits);
