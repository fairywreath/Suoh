#include <iostream>

#include <GLFW/glfw3.h>

#include <RenderCore/RenderCore.h>
#include <RenderCore/Vulkan/VulkanDevice.h>

#include <Logger.h>

static constexpr auto FRAMEBUFFER_WIDTH = 1280;
static constexpr auto FRAMEBUFFER_HEIGHT = 720;

GLFWwindow* g_Window;

void WindowInit()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto width = FRAMEBUFFER_WIDTH;
    auto height = FRAMEBUFFER_HEIGHT;

    g_Window = glfwCreateWindow(width, height, "Test Suoh App", nullptr, nullptr);
    if (g_Window == nullptr)
    {
        LOG_ERROR("Failed to create window!");
        glfwTerminate();
        abort();
    }
}

void WindowDestroy()
{
    glfwDestroyWindow(g_Window);
    glfwTerminate();

    g_Window = nullptr;
}

void WindowLoop()
{
    while (!glfwWindowShouldClose(g_Window))
    {
        glfwPollEvents();
    }
}

using namespace RenderCore;

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting application...");

    WindowInit();

    RenderCore::DeviceDesc deviceDesc;
    deviceDesc.framebufferWidth = FRAMEBUFFER_WIDTH;
    deviceDesc.framebufferHeight = FRAMEBUFFER_HEIGHT;
    deviceDesc.glfwWindowPtr = g_Window;

    auto renderDevice = RenderCore::CreateVulkanDevice(deviceDesc);

    {
        auto buf1 = renderDevice->CreateBuffer({
            .size = 1024,
            .usage = BufferUsage::STORAGE_BUFFER,
        });

        auto cmd = renderDevice->CreateCommandList({});
    }

    WindowLoop();

    WindowDestroy();

    return 0;
}
