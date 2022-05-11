#include <iostream>

#include <Logger.h>

#include "Application.h"

// #include <Renderer/Vulkan/VKRenderDevice.h>
#include <Renderer/Vulkan/VKCommon.h>

#include <cassert>

using namespace Suou;

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting client...");

    Application app;
    app.run();

    return 0;
}
