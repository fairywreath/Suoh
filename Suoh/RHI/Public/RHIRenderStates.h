#pragma once

#include "RHICommon.h"
#include "RHIContainers.h"

#include <string>

#ifdef WIN32
#define PRAGMA_NO_PADDING_START __pragma(pack(push, 1))
#define PRAGMA_NO_PADDING_END __pragma(pack(pop))
#else
static_assert(false, "PRAGMA_NO_PADDING implementation required for this platform!");
#endif

namespace SuohRHI
{

static constexpr auto MAX_RENDER_TARGETS = 8;

PRAGMA_NO_PADDING_START;

enum class GraphicsAPI : u8
{
    VULKAN,
};

/*
 * Data types.
 */
struct Color
{
    Color() : r(0.0f), g(0.0f), b(0.0f), a(0.0f)
    {
    }

    Color(float val) : r(val), g(val), b(val), a(val)
    {
    }

    Color(float rVal, float gVal, float bVal, float aVal) : r(rVal), g(gVal), b(bVal), a(aVal)
    {
    }

    bool operator==(const Color& other) const
    {
        return (r == other.r) && (g == other.g) && (b == other.b) && (a == other.a);
    }

    bool operator!=(const Color& other) const
    {
        return !(*this == other);
    }

    float r;
    float g;
    float b;
    float a;
};

struct Viewport
{
    f32 topLeftX{0};
    f32 topLeftY{0};
    f32 width{0};
    f32 height{0};
    f32 minDepth{0};
    f32 maxDepth{0};
};

struct Rect
{
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};

/*
 * Image formats.
 */
enum class Format : u32
{
    UNKNOWN,

    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R16G16B16A16_FLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_SINT,
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    R11G11B10_FLOAT,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8B8A8_UINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_SINT,

    B8G8R8A8_UNORM,
    B8G8R8A8_UNORM_SRGB,
    B8G8R8A8_SNORM,
    B8G8R8A8_UINT,
    B8G8R8A8_SINT,

    R16G16_FLOAT,
    R16G16_UNORM,
    R16G16_UINT,
    R16G16_SNORM,
    R16G16_SINT,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,
    R8G8_UNORM,
    R8G8_UINT,
    R8G8_SNORM,
    R8G8_SINT,
    R16_FLOAT,
    D16_UNORM,
    R16_UNORM,
    R16_UINT,
    R16_SNORM,
    R16_SINT,
    R8_UNORM,
    R8_UINT,
    R8_SNORM,
    R8_SINT
};

enum class FormatType : u8
{
    FLOAT,
    DEPTH_STENCIL,
    INTEGER,
    NORMALIZED,
};

enum class FormatSupport : u32
{
    NONE = 0x00000000,

    BUFFER = 0x00000001,
    INDEX_BUFFER = 0x00000002,
    VERTEX_BUFFER = 0x00000004,
    TEXTURE = 0x00000008,
    DEPTH_STENCIL = 0x00000010,
    RENDER_TARGET = 0x00000020,
    BLENDABLE = 0x00000040,
};

SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(FormatSupport);

/*
 * Heap.
 */
enum class HeapType : u8
{
    DEVICE,
    UPLOAD,
    READBACK,
};

struct MemoryRequirements
{
    u64 size{0};
    u64 alignment{0};
};

/*
 * Texture.
 */
enum class TextureDimension : u8
{
    UNKNOWN,
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_CUBE,
    TEXTURE_CUBE_ARRAY,
    TEXTURE_2DMS,
    TEXTURE_2DMS_ARRAY,
    TEXTURE_3D
};

enum class CpuAccessMode : u8
{
    NONE,
    READ,
    WRITE
};

enum class ResourceStates : u32
{
    UNKNOWN = 0X00000000,
    COMMON = 0X00000001,
    CONSTANT_BUFFER = 0X00000002,
    VERTEX_BUFFER = 0X00000004,
    INDEX_BUFFER = 0X00000008,
    INDIRECT_ARGUMENT = 0X00000010,
    SHADER_RESOURCE = 0X00000020,
    UNORDERED_ACCESS = 0X00000040,
    RENDER_TARGET = 0X00000080,
    DEPTH_WRITE = 0X00000100,
    DEPTH_READ = 0X00000200,
    STREAM_OUT = 0X00000400,
    COPY_DEST = 0X00000800,
    COPY_SOURCE = 0X00001000,
    RESOLVE_DEST = 0X00002000,
    RESOLVE_SOURCE = 0X00004000,
    PRESENT = 0X00008000,
    ACCEL_STRUCT_READ = 0X00010000,
    ACCEL_STRUCT_WRITE = 0X00020000,
    ACCEL_STRUCT_BUILD_INPUT = 0X00040000,
    ACCEL_STRUCT_BUILD_BLAS = 0X00080000,
    SHADING_RATE_SURFACE = 0X00100000,
};

SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(ResourceStates);

using MipLevel = u32;
using ArrayLayer = u32;

/*
 * Input Formats.
 */
enum class InputFormat : u32
{
    UNKNOWN,

    R32G32B32A32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R32G32_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R32_FLOAT,
    R32_UINT,
    R32_SINT,

    R16G16B16A16_FLOAT,
    R16G16B16A16_UINT,
    R16G16B16A16_SINT,
    R16G16_FLOAT,
    R16G16_UINT,
    R16G16_SINT,
    R16_FLOAT,
    R16_UINT,
    R16_SINT,

    R8G8B8A8_UNORM,
    R8G8B8A8_UINT,
    R8G8B8A8_SINT,
    R8G8_UINT,
    R8G8_SINT,
    R8_UINT,
    R8_SINT,
};

enum class InputClassification : u8
{
    PER_VERTEX,
    PER_INSTANCE
};

static constexpr auto INPUT_LAYOUT_NAME_MAX_LENGTH = 16;
struct InputLayout
{
    bool enabled = false;
    u32 index = 0;
    InputFormat format = InputFormat::UNKNOWN;
    u32 slot = 0;
    u32 alignedByteOffset = 0;
    InputClassification inputClassification = InputClassification::PER_VERTEX;
    u32 instanceDataStepRate = 0;

    const char* GetName() const
    {
        return _name;
    }
    void SetName(const std::string& name)
    {
        memset(_name, 0, INPUT_LAYOUT_NAME_MAX_LENGTH);
        strcpy_s(_name, name.data());
    }

private:
    char _name[INPUT_LAYOUT_NAME_MAX_LENGTH]{};
};

struct FormatInfo
{
    std::string name;
    u8 bytesPerBlock;
    u8 blockSize;

    Format format;
    FormatType formatType;

    bool hasRed : 1;
    bool hasGreen : 1;
    bool hasBlue : 1;
    bool hasAlpha : 1;
    bool hasDepth : 1;
    bool hasStencil : 1;
    bool isSigned : 1;
    bool isRGB : 1;
};

/*
 * Buffer.
 */
enum class BufferUsage : u32
{
    UNKNOWN = 0X00000000,
    STORAGE_BUFFER = 0X00000001,
    VERTEX_BUFFER = 0X00000002,
    INDEX_BUFFER = 0X00000004,
    UNIFORM_BUFFER = 0X00000008,
    INDIRECT_BUFFER = 0X0000010,
    TRANSFER_SOURCE = 0X00000020,
    TRANSFER_DESTINATION = 0X00000040,
};

SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(BufferUsage);

/*
 * Shaders.
 */
enum class ShaderType : u32
{
    NONE = 0X00000000,

    VERTEX = 0X00000001,
    FRAGMENT = 0X00000002,
    GEOMETRY = 0X00000004,

    TESSELLATION_CONTROL = 0X00000010,
    TESSELLATION_EVALUATION = 0X00000020,

    COMPUTE = 0X00000100,
};

SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(ShaderType);

/*
 * Blend states.
 */
enum class BlendMode : u8
{
    ZERO,
    ONE,
    SRC_COLOR,
    INV_SRC_COLOR,
    SRC_ALPHA,
    INV_SRC_ALPHA,
    DEST_ALPHA,
    INV_DEST_ALPHA,
    DEST_COLOR,
    INV_DEST_COLOR,
    SRC_ALPHA_SAT,
    BLEND_FACTOR,
    INV_BLEND_FACTOR,
    SRC1_COLOR,
    INV_SRC1_COLOR,
    SRC1_ALPHA,
    INV_SRC1_ALPHA
};

enum class BlendOp : u8
{
    ADD,
    SUBTRACT,
    REV_SUBTRACT,
    MIN,
    MAX
};

enum class LogicOp : u8
{
    CLEAR,
    SET,
    COPY,
    COPY_INVERTED,
    NOOP,
    INVERT,
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    EQUIV,
    AND_REVERSE,
    AND_INVERTED,
    OR_REVERSE,
    OR_INVERTED,
};

enum class ColorMask : u8
{
    RED = 0x1,
    GREEN = 0x2,
    BLUE = 0x4,
    ALPHA = 0x8,
    ALL = 0xF,
};

SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(ColorMask);

// Render target blend state
struct RTBlendState
{
    bool blendEnable{false};
    bool logicOpEnable{false};

    BlendMode srcBlend{BlendMode::ONE};
    BlendMode destBlend{BlendMode::ZERO};
    BlendOp blendOp{BlendOp::ADD};

    BlendMode srcBlendAlpha{BlendMode::ONE};
    BlendMode destBlendAlpha{BlendMode::ZERO};
    BlendOp blendOpAlpha{BlendOp::ADD};

    LogicOp logicOp{LogicOp::NOOP};
    ColorMask renderTargetWriteMask{ColorMask::ALL};
};

struct BlendState
{
    RTBlendState renderTargets[MAX_RENDER_TARGETS];
    bool alphaToCoverageEnable{false};
    bool independentBlendEnable{false};
};

/*
 * Depth/Stencil.
 */
enum class ComparisonFunc : u8
{
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
};

enum class StencilOp : u8
{
    KEEP,
    ZERO,
    REPLACE,
    INCR_SAT,
    DECR_SAT,
    INVERT,
    INCR,
    DECR
};

struct StencilOpDesc
{
    StencilOp stencilFailOp{StencilOp::KEEP};
    StencilOp stencilDepthFailOp{StencilOp::KEEP};
    StencilOp stencilPassOp{StencilOp::KEEP};
    ComparisonFunc stencilFunc{ComparisonFunc::ALWAYS};
};

struct DepthStencilState
{
    bool depthTestEnable{false};
    bool depthWriteEnable{false};
    ComparisonFunc depthFunc{ComparisonFunc::LESS};

    bool stencilEnable{false};
    u8 stencilReadMask{0xff};
    u8 stencilWriteMask{0xff};

    StencilOpDesc frontFaceStencil;
    StencilOpDesc backFaceStencil;
};

enum class DepthClearFlags : u8
{
    DEPTH,
    STENCIL,
    BOTH
};

struct DepthBias
{
    f32 constantFactor;
    f32 clamp;
    f32 slopeFactor;
};

/*
 * Viewports.
 */
static constexpr auto MAX_VIEWPORTS = 8;
struct ViewportState
{
    static_vector<Viewport, MAX_VIEWPORTS> viewports;
    static_vector<Rect, MAX_VIEWPORTS> scissorRects;
};

/*
 * Rasterizer State.
 */
enum class FillMode : u8
{
    SOLID,
    WIREFRAME
};

enum class CullMode : u8
{
    NONE,
    FRONT,
    BACK
};

enum class FrontFaceState : u8
{
    CLOCKWISE,
    COUNTERCLOCKWISE
};

enum class SampleCount : u8
{
    SAMPLE_COUNT_1,
    SAMPLE_COUNT_2,
    SAMPLE_COUNT_4,
    SAMPLE_COUNT_8
};

constexpr int SampleCountToInt(SampleCount sampleCount)
{
    switch (sampleCount)
    {
    case SampleCount::SAMPLE_COUNT_1:
        return 1;
    case SampleCount::SAMPLE_COUNT_2:
        return 2;
    case SampleCount::SAMPLE_COUNT_4:
        return 4;
    case SampleCount::SAMPLE_COUNT_8:
        return 8;
    default:
        assert("Invalid SamleCount enum!");
    }
    return 0;
}

struct RasterizerState
{
    FillMode fillMode{FillMode::SOLID};
    CullMode cullMode{CullMode::BACK};

    FrontFaceState frontFaceState{FrontFaceState::CLOCKWISE};
    bool depthBiasEnabled{false};
    i32 depthBias{0};
    f32 depthBiasClamp{0.0f};
    f32 depthBiasSlopeFactor{0.0f};

    bool scissorEnabled{false};
    bool antiAliasedLineEnabled{false};

    SampleCount sampleCount{SampleCount::SAMPLE_COUNT_1};
};

enum class PrimitiveTopology : u8
{
    TRIANGLES,
    TRIANGLE_STRIP
};

/*
 * Sampler
 */
enum class SamplerFilter : u32
{
    MIN_MAG_MIP_POINT,
    MIN_MAG_POINT_MIP_LINEAR,
    MIN_POINT_MAG_LINEAR_MIP_POINT,
    MIN_POINT_MAG_MIP_LINEAR,
    MIN_LINEAR_MAG_MIP_POINT,
    MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MIN_MAG_LINEAR_MIP_POINT,
    MIN_MAG_MIP_LINEAR,
    ANISOTROPIC,
    COMPARISON_MIN_MAG_MIP_POINT,
    COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
    COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
    COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
    COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
    COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
    COMPARISON_MIN_MAG_MIP_LINEAR,
    COMPARISON_ANISOTROPIC,
    MINIMUM_MIN_MAG_MIP_POINT,
    MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
    MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
    MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
    MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
    MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
    MINIMUM_MIN_MAG_MIP_LINEAR,
    MINIMUM_ANISOTROPIC,
    MAXIMUM_MIN_MAG_MIP_POINT,
    MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
    MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
    MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
    MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
    MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
    MAXIMUM_MIN_MAG_MIP_LINEAR,
    MAXIMUM_ANISOTROPIC
};

enum class SamplerReductionMode : u8
{
    NONE,
    MAX,
    MIN
};

enum class StaticBorderColor : u8
{
    TRANSPARENT_BLACK,
    OPAQUE_BLACK,
    OPAQUE_WHITE
};

enum class TextureAddressMode : u8
{
    WRAP,
    MIRROR,
    CLAMP,
    BORDER,
    MIRROR_ONCE
};

enum class ShaderVisibility : u8
{
    NONE,
    VERTEX,
    GEOMETRY,
    FRAGMENT,
    ALL,
};

struct Sampler
{
    bool enabled{false};

    TextureAddressMode addressU{TextureAddressMode::CLAMP};
    TextureAddressMode addressV{TextureAddressMode::CLAMP};
    TextureAddressMode addressW{TextureAddressMode::CLAMP};

    SamplerFilter filter{SamplerFilter::MIN_MAG_MIP_POINT};
    SamplerReductionMode mode{SamplerReductionMode::NONE};

    f32 mipLODBias{0.0f};
    u32 maxAnisotropy{0};
    bool comparisonEnabled{false};
    ComparisonFunc comparisonFunc{ComparisonFunc::ALWAYS};
    StaticBorderColor borderColor{StaticBorderColor::OPAQUE_BLACK};

    f32 minLOD{0.0f};
    f32 maxLOD{std::numeric_limits<f32>::max()};

    ShaderVisibility shaderVisibility{ShaderVisibility::FRAGMENT};
    bool unnormalizedCoordinates{false};

    std::string debugName;
};

/*
 * Bindings.
 */
enum class BindingResourceType : u8
{
    NONE,
    CONSTANT_BUFFER,
    SAMPLER,
    PUSH_CONSTANTS,
    COUNT
};

/*
 * Graphics pipeline/draw states.
 */
struct SinglePassStereoState
{
    bool enable{false};
    bool independentViewportMask{false};
    u16 renderTargetIndexOffset{0};
};

struct RenderState
{
    BlendState blendState;
    DepthStencilState depthStencilState;
    RasterizerState rasterizerState;
    SinglePassStereoState singlePassStereoState;
};

PRAGMA_NO_PADDING_END;

} // namespace SuohRHI