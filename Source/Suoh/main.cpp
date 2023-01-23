#include <iostream>

#include <Core/Logger.h>
#include <Core/SystemCommand.h>

#include "Application/Window.h"

#include "Graphics/SPIRV/ShaderCompiler.h"
#include "Graphics/Vulkan/VulkanDevice.h"

int main()
{
    LOG_SET_OUTPUT(&std::cout);

    LOG_TRACE("Starting application...");

    WindowConfiguration windowConfig = {
        .width = 1600,
        .height = 900,
        .name = "Suoh Engine",
    };
    Window window(windowConfig);

    VulkanDeviceDesc deviceDesc = {
        .window = window.GetNativeWindowHandle(),
        .width = windowConfig.width,
        .height = windowConfig.height,
        .useDynamicRendering = false,
    };
    VulkanDevice device(deviceDesc);
    LOG_INFO("GPU: ", device.GetGPUName());

    ShaderStateDesc shaderDesc;
    shaderDesc.name = "Test shader state";

    ShaderStage shaderStage1;
    shaderStage1.spirvInput = false;
    shaderStage1.readFromFile = true;
    shaderStage1.data = "Shaders/test2.vert.glsl";
    shaderStage1.stageFlags = vk::ShaderStageFlagBits::eVertex;
    shaderDesc.shaderStages.push_back(shaderStage1);

    auto shaderState1 = device.CreateShaderState(shaderDesc);
    assert(shaderState1 != nullptr);

    while (!window.ShouldClose())
    {
        // device.NewFrame();
        // device.Present();

        window.HandleEvents();
    }

    LOG_TRACE("Closing application...");

    return 0;
}