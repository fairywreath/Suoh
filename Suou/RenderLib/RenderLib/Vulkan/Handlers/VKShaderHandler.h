#pragma once

#include <memory>

#include "SuouBase.h"

#include "RenderLib/Resources/Shader.h"

namespace Suou
{

class VKRenderDevice;

struct IVKShaderHandlerData
{
};

class VKShaderHandler
{
public:
    explicit VKShaderHandler(VKRenderDevice& renderDevice);
    ~VKShaderHandler();

    SUOU_NON_COPYABLE(VKShaderHandler);
    SUOU_NON_MOVEABLE(VKShaderHandler);

    ShaderHandle createShader(const ShaderDescription& desc);
    void destroyShader(ShaderHandle handle);

private:
    using HandleType = type_safe::underlying_type<ShaderHandle>;
    constexpr HandleType toHandleType(ShaderHandle handle) const
    {
        return static_cast<HandleType>(handle);
    }
    ShaderHandle acquireNewHandle();

private:
    VKRenderDevice& mRenderDevice;

    std::unique_ptr<IVKShaderHandlerData> mData;
};

} // namespace Suou