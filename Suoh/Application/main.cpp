#include <iostream>

#include <Logger.h>

#include "VulkanRHI.h"

#include <GLFW/glfw3.h>

/*
 *Simple Test App with a window.
 */

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
        // glfwSwapBuffers(g_Window);
        glfwPollEvents();
    }
}

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting application...");

    WindowInit();

    SuohRHI::Vulkan::DeviceDesc deviceDesc;
    deviceDesc.glfwWindowPtr = g_Window;

    auto deviceHandle = SuohRHI::Vulkan::CreateDevice(deviceDesc);

    WindowLoop();

    WindowDestroy();

    return 0;
}