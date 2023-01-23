#include "GPUStates.h"

#include <array>

namespace
{

// using namespace std::string_view_literals;

/*
 * XXX: Hardcoded :( enum state names in the case we need them.
 */

static constexpr auto UnsupportedString = "Unsupported";

static constexpr std::array BlendModeNames = {
    "Zero",         "One",          "SrcColor",     "InvSrcColor",  "SrcAlpha",    "InvSrcAlpha",
    "DestAlpha",    "InvDestAlpha", "DestColor",    "InvDestColor", "SrcAlphaSat", "Src1Color",
    "InvSrc1Color", "Src1Alpha",    "InvSrc1Alpha", "Count",
};
static_assert(static_cast<u32>(BlendMode::Count) == BlendModeNames.size() - 1);

static constexpr std::array BlendOperationNames = {
    "Add", "Subtract", "RevSubtract", "Min", "Max", "Count",
};

static constexpr std::array ColorWriteEnableNames = {
    "Red", "Green", "Blue", "Alpha", "All", "Count",
};

static constexpr std::array ComparisonFunctionNames = {
    "Never", "Less", "Equal", "LessEqual", "Greater", "NotEqual", "GreaterEqual", "Always", "Count",
};

static constexpr std::array CullModeNames = {"None", "Front", "Back", "Count"};

static constexpr std::array DepthWriteNames = {"Zero", "All", "Count"};

static constexpr std::array FillModeNames = {"Wireframe", "Solid", "Point", "Count"};

static constexpr std::array FrontClockWiseNames = {"True", "False", "Count"};

static constexpr std::array StencilOperationNames = {
    "Keep", "Zero", "Replace", "IncrSat", "DecrSat", "Invert", "Incr", "Decr", "Count",
};

static constexpr std::array TextureFormatNames = {
    "UNKNOWN",
    "R32G32B32A32_TYPELESS",
    "R32G32B32A32_FLOAT",
    "R32G32B32A32_UINT",
    "R32G32B32A32_SINT",
    "R32G32B32_TYPELESS",
    "R32G32B32_FLOAT",
    "R32G32B32_UINT",
    "R32G32B32_SINT",
    "R16G16B16A16_TYPELESS",
    "R16G16B16A16_FLOAT",
    "R16G16B16A16_UNORM",
    "R16G16B16A16_UINT",
    "R16G16B16A16_SNORM",
    "R16G16B16A16_SINT",
    "R32G32_TYPELESS",
    "R32G32_FLOAT",
    "R32G32_UINT",
    "R32G32_SINT",
    "R10G10B10A2_TYPELESS",
    "R10G10B10A2_UNORM",
    "R10G10B10A2_UINT",
    "R11G11B10_FLOAT",
    "R8G8B8A8_TYPELESS",
    "R8G8B8A8_UNORM",
    "R8G8B8A8_UNORM_SRGB",
    "R8G8B8A8_UINT",
    "R8G8B8A8_SNORM",
    "R8G8B8A8_SINT",
    "R16G16_TYPELESS",
    "R16G16_FLOAT",
    "R16G16_UNORM",
    "R16G16_UINT",
    "R16G16_SNORM",
    "R16G16_SINT",
    "R32_TYPELESS",
    "R32_FLOAT",
    "R32_UINT",
    "R32_SINT",
    "R8G8_TYPELESS",
    "R8G8_UNORM",
    "R8G8_UINT",
    "R8G8_SNORM",
    "R8G8_SINT",
    "R16_TYPELESS",
    "R16_FLOAT",
    "R16_UNORM",
    "R16_UINT",
    "R16_SNORM",
    "R16_SINT",
    "R8_TYPELESS",
    "R8_UNORM",
    "R8_UINT",
    "R8_SNORM",
    "R8_SINT",
    "R9G9B9E5_SHAREDEXP",
    "D32_FLOAT_S8X24_UINT",
    "D24_UNORM_S8_UINT",
    "D32_FLOAT",
    "D24_UNORM_X8_UINT",
    "D16_UNORM",
    "S8_UINT",
    "BC1_TYPELESS",
    "BC1_UNORM",
    "BC1_UNORM_SRGB",
    "BC2_TYPELESS",
    "BC2_UNORM",
    "BC2_UNORM_SRGB",
    "BC3_TYPELESS",
    "BC3_UNORM",
    "BC3_UNORM_SRGB",
    "BC4_TYPELESS",
    "BC4_UNORM",
    "BC4_SNORM",
    "BC5_TYPELESS",
    "BC5_UNORM",
    "BC5_SNORM",
    "B5G6R5_UNORM",
    "B5G5R5A1_UNORM",
    "B8G8R8A8_UNORM",
    "B8G8R8X8_UNORM",
    "R10G10B10_XR_BIAS_A2_UNORM",
    "B8G8R8A8_TYPELESS",
    "B8G8R8A8_UNORM_SRGB",
    "B8G8R8X8_TYPELESS",
    "B8G8R8X8_UNORM_SRGB",
    "BC6H_TYPELESS",
    "BC6H_UF16",
    "BC6H_SF16",
    "BC7_TYPELESS",
    "BC7_UNORM",
    "BC7_UNORM_SRGB",
    "FORCE_UINT",
    "Count",
};

static constexpr std::array PrimitiveTopologyNames = {
    "Unknown", "Point", "Line", "Triangle", "Patch", "Count",
};

static constexpr std::array BufferUsageNames = {
    "Vertex", "Index", "Constant", "Indirect", "Structured", "Count",
};

static constexpr std::array ResourceUsageTypeNames = {
    "Immutable", "Dynamic", "Stream", "Staging", "Count",
};

static constexpr std::array IndexTypeNames = {"Uint16", "Uint32", "Count"};

static constexpr std::array TextureTypeNames = {
    "Texture1D",        "Texture2D",          "Texture3D", "Texture_1D_Array",
    "Texture_2D_Array", "Texture_Cube_Array", "Count",
};

static constexpr std::array TextureFilterNames = {"Nearest", "Linear", "Count"};

static constexpr std::array TextureMipFilterNames = {"Nearest", "Linear", "Count"};

static constexpr std::array TextureAddressModeNames = {
    "Repeat", "Mirrored_Repeat", "Clamp_Edge", "Clamp_Border", "Count",
};

static constexpr std::array VertexComponentFormatNames = {
    "Float",  "Float2",  "Float3", "Float4",  "Mat4", "Byte",  "Byte4N", "UByte", "UByte4N",
    "Short2", "Short2N", "Short4", "Short4N", "Uint", "Uint2", "Uint4",  "Count",
};

static constexpr std::array VertexInputRateNames = {"PerVertex", "PerInstance", "Count"};

static constexpr std::array LogicOperationNames = {
    "Clear",      "Set",         "Copy",      "CopyInverted", "Noop",  "Invert",
    "And",        "Nand",        "Or",        "Nor",          "Xor",   "Equiv",
    "AndReverse", "AndInverted", "OrReverse", "OrInverted",   "Count",
};

static constexpr std::array QueueTypeNames = {"Graphics", "Compute", "Copy", "Count"};

static constexpr std::array CommandTypeNames = {
    "BindPipeline",
    "BindResourceTable",
    "BindVertexBuffer",
    "BindIndexBuffer",
    "BindResourceSet",
    "Draw",
    "DrawIndexed",
    "DrawInstanced",
    "DrawIndexedInstanced",
    "Dispatch",
    "CopyResource",
    "SetScissor",
    "SetViewport",
    "Clear",
    "ClearDepth",
    "ClearStencil",
    "BeginRenderPass",
    "EndRenderPass",
    "Count",
};

static constexpr std::array TextureFlagsNames = {"Default", "RenderTarget", "Compute", "Count"};

static constexpr std::array PresentModeNames = {
    "Immediate", "VSync", "VSyncFast", "VSyncRelaxed", "Count",
};

} // namespace

std::string_view BlendModeName(BlendMode blendMode)
{
    u32 val = static_cast<u32>(blendMode);
    if (val < BlendModeNames.size())
    {
        return BlendModeNames[val];
    }
    else
    {
        return UnsupportedString;
    }
}