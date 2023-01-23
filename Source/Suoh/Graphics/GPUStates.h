#pragma once

#include <string_view>

#include <Core/Types.h>

struct Rect2D
{
    f32 x{0.0f};
    f32 y{0.0f};
    f32 width{0.0f};
    f32 height{0.0f};
};

struct Rect2DInt
{
    i32 x{0};
    i32 y{0};
    u32 width{0};
    u32 height{0};
};

struct Viewport
{
    Rect2DInt rect{};
    f32 minDepth{0.0f};
    f32 maxDepth{0.0f};
};

struct ViewportState
{
    std::vector<Viewport> viewports;
    std::vector<Rect2DInt> scissors;
};

// XXX: Use a better type-safe flag masking mechanism(similar to vulkan.hpp)?
#define ENUM_CLASS_FLAG_OPERATORS(T)                                                               \
    inline T operator|(T a, T b)                                                                   \
    {                                                                                              \
        return static_cast<T>(static_cast<std::underlying_type<T>::type>(a)                        \
                              | static_cast<std::underlying_type<T>::type>(b));                    \
    }                                                                                              \
    inline bool operator&(T a, T b)                                                                \
    {                                                                                              \
        return (static_cast<std::underlying_type<T>::type>(a)                                      \
                & static_cast<std::underlying_type<T>::type>(b));                                  \
    }                                                                                              \
    inline T operator~(T a)                                                                        \
    {                                                                                              \
        return static_cast<T>(~static_cast<std::underlying_type<T>::type>(a));                     \
    }                                                                                              \
    inline bool operator!(T a)                                                                     \
    {                                                                                              \
        return static_cast<std::underlying_type<T>::type>(a) == 0;                                 \
    }                                                                                              \
    inline bool operator==(T a, u32 b)                                                             \
    {                                                                                              \
        return static_cast<std::underlying_type<T>::type>(a) == b;                                 \
    }                                                                                              \
    inline bool operator!=(T a, u32 b)                                                             \
    {                                                                                              \
        return static_cast<std::underlying_type<T>::type>(a) != b;                                 \
    }

enum class BlendMode
{
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphasat,
    Src1Color,
    InvSrc1Color,
    Src1Alpha,
    InvSrc1Alpha,
    Count,
};
std::string_view BlendModeName(BlendMode blendMode);

enum class BlendOperation
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max,
    Count,
};

enum class ColorWriteEnableFlags : u8
{
    Red = 0x01,
    Green = 0x02,
    Blue = 0x04,
    Alpha = 0x08,
    All = 0xFF,
};
ENUM_CLASS_FLAG_OPERATORS(ColorWriteEnableFlags);

enum class ComparisonFunction
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
    Count,
};

enum class CullMode
{
    None,
    Front,
    Back,
    Count,
};

enum class DepthWriteFlags : u8
{
    Zero = 0x01,
    All = 0xFF,
};
ENUM_CLASS_FLAG_OPERATORS(DepthWriteFlags);

enum class FillMode
{
    Wireframe,
    Solid,
    Point,
    Count,
};

enum class FrontClockWise
{
    True,
    False,
    Count,
};

enum class StencilOperation
{
    Keep,
    Zero,
    Replace,
    IncrSat,
    DecrSat,
    Invert,
    Incr,
    Decr,
    Count,
};

enum class TextureFormat
{
    UNKNOWN,
    R32G32B32A32_TYPELESS,
    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32_TYPELESS,
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R16G16B16A16_TYPELESS,
    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,
    R32G32_TYPELESS,
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R10G10B10A2_TYPELESS,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R11G11B10_FLOAT,
    R8G8B8A8_TYPELESS,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,
    R16G16_TYPELESS,
    R16G16_FLOAT,
    R16G16_UNORM,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,
    R32_TYPELESS,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,
    R8G8_TYPELESS,
    R8G8_UNORM,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,
    R16_TYPELESS,
    R16_FLOAT,
    R16_UNORM,
    R16_UINT,
    R16_SNORM,
    R16_SINT,
    R8_TYPELESS,
    R8_UNORM,
    R8_UINT,
    R8_SNORM,
    R8_SINT,
    R9G9B9E5_SHAREDEXP,
    D32_FLOAT_S8X24_UINT,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D24_UNORM_X8_UINT,
    D16_UNORM,
    S8_UINT,
    BC1_TYPELESS,
    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_TYPELESS,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_TYPELESS,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_TYPELESS,
    BC4_UNORM,
    BC4_SNORM,
    BC5_TYPELESS,
    BC5_UNORM,
    BC5_SNORM,
    B5G6R5_UNORM,
    B5G5R5A1_UNORM,
    B8G8R8A8_UNORM,
    B8G8R8X8_UNORM,
    R10G10B10_XR_BIAS_A2_UNORM,
    B8G8R8A8_TYPELESS,
    B8G8R8A8_UNORM_SRGB,
    B8G8R8X8_TYPELESS,
    B8G8R8X8_UNORM_SRGB,
    BC6H_TYPELESS,
    BC6H_UF16,
    BC6H_SF16,
    BC7_TYPELESS,
    BC7_UNORM,
    BC7_UNORM_SRGB,
    FORCE_UINT,
    Count,
};

enum class PrimitiveTopology
{
    Unknown,
    Point,
    Line,
    Triangle,
    Patch,
    Count,
};

enum class BufferUsage
{
    Vertex,
    Index,
    Constant,
    Indirect,
    Structured,
    Count,
};

enum class ResourceUsageType
{
    Immutable,
    Dynamic,
    Stream,
    Staging,
    Count,
};

enum class IndexType
{
    Uint16,
    Uint32,
    Count,
};

enum class TextureType
{
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    Count,
};

enum class TextureFilter
{
    Nearest,
    Linear,
    Count,
};

enum class TextureMipFilter
{
    Nearest,
    Linear,
    Count,
};

enum class TextureAddressMode
{
    Repeat,
    Mirrored_Repeat,
    Clamp_Edge,
    Clamp_Border,
    Count,
};

enum class VertexComponentFormat
{
    Float,
    Float2,
    Float3,
    Float4,
    Mat4,
    Byte,
    Byte4N,
    UByte,
    UByte4N,
    Short2,
    Short2N,
    Short4,
    Short4N,
    Uint,
    Uint2,
    Uint4,
    Count,
};

enum class VertexInputRate
{
    PerVertex,
    PerInstance,
    Count,
};

enum class LogicOperation
{
    Clear,
    Set,
    Copy,
    CopyInverted,
    Noop,
    Invert,
    And,
    Nand,
    Or,
    Nor,
    Xor,
    Equiv,
    AndReverse,
    AndInverted,
    OrReverse,
    OrInverted,
    Count,
};

enum class PipelineType
{
    Graphics,
    Compute,
    RayTracing,
    Count,
};

enum class ShaderStageFlags : u32
{
    Vertex = 0x00000001,
    Fragment = 0x00000002,
    Compute = 0x00000004,
    Task = 0x00000008,
    Mesh = 0x00000010,
    All = 0xFFFFFFFF,
};
ENUM_CLASS_FLAG_OPERATORS(ShaderStageFlags);

enum class QueueType
{
    Graphics,
    Compute,
    Transfer,
    Count,
};

enum class CommandType
{
    BindPipeline,
    BindResourceTable,
    BindVertexBuffer,
    BindIndexBuffer,
    BindResourceSet,
    Draw,
    DrawIndexed,
    DrawInstanced,
    DrawIndexedInstanced,
    Dispatch,
    CopyResource,
    SetScissor,
    SetViewport,
    Clear,
    ClearDepth,
    ClearStencil,
    BeginRenderPass,
    EndRenderPass,
    Count,
};

enum class DeviceExtensionsFlags : u8
{
    DebugCallback = 0x01,
    All = 0xFF,
};
ENUM_CLASS_FLAG_OPERATORS(DeviceExtensionsFlags);

enum class TextureFlags : u8
{
    Default = 0x01,
    RenderTarget = 0x02,
    Compute = 0x04
};
ENUM_CLASS_FLAG_OPERATORS(TextureFlags);

enum class PipelineStage
{
    DrawIndirect = 0,
    VertexInput = 1,
    VertexShader = 2,
    FragmentShader = 3,
    RenderTarget = 4,
    ComputeShader = 5,
    Transfer = 6,
};

enum class ResourceType
{
    Buffer,
    Texture,
    Pipeline,
    Sampler,
    DescriptorSetLayout,
    DescriptorSet,
    RenderPass,
    Framebuffer,
    ShaderState,
    Count,
};

enum class PresentMode
{
    Immediate,
    VSync,
    VSyncFast,
    VSyncRelaxed,
    Count,
};

enum class RenderPassOperation
{
    DontCare,
    Load,
    Clear,
    Count,
};

enum class ResourceState : u32
{
    Undefined = 0x0,
    VertexAndUniformBuffer = 0x1,
    IndexBuffer = 0x2,
    RenderTarget = 0x4,
    UnorderedAccess = 0x8,
    DepthWrite = 0x10,
    DepthRead = 0x20,
    NonFragmentShaderResource = 0x40,
    FragmentShaderResource = 0x80,
    ShaderResource = 0x40 | 0x80,
    StreamOut = 0x100,
    IndirectArgument = 0x200,
    CopyDest = 0x400,
    CopySrc = 0x800,
    GenericRead = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
    Present = 0x1000,
    Common = 0x2000,
    RaytracingAccelerationStructure = 0x4000,
    ShadingRateResource = 0x8000,
};
