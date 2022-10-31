#include <iostream>

#include <GLFW/glfw3.h>

#include <RenderLib/RenderLib.h>
#include <RenderLib/Vulkan/VulkanDevice.h>

#include <Logger.h>

#include <RenderDescription/Material.h>
#include <RenderDescription/Mesh.h>
#include <RenderDescription/Scene.h>
#include <RenderDescription/Utils.h>

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

using namespace RenderLib;
using namespace RenderLib::Vulkan;

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting application...");

    WindowInit();

    DeviceDesc deviceDesc;
    deviceDesc.framebufferWidth = FRAMEBUFFER_WIDTH;
    deviceDesc.framebufferHeight = FRAMEBUFFER_HEIGHT;
    deviceDesc.glfwWindowPtr = g_Window;

    auto renderDevice = CreateVulkanDevice(deviceDesc);

    auto vs = renderDevice->CreateShader({
        .fileName = "../../../Shaders/SimpleVertex.vert.spv",
        .needsCompilation = false,
    });
    auto fs = renderDevice->CreateShader({
        .fileName = "../../../Shaders/SimpleFragment.frag.spv",
        .needsCompilation = false,
    });

    std::vector<vec3> positions = {
        {1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
    };

    auto vertexBufferDesc = BufferDesc{};
    vertexBufferDesc.size = sizeof(vec3) * positions.size();
    vertexBufferDesc.usage = BufferUsage::STORAGE_BUFFER;
    auto storageBuffer = renderDevice->CreateBuffer(vertexBufferDesc);

    // Upload data to storage buffer.
    auto transferCommandList
        = renderDevice->CreateCommandList({.usage = CommandListUsage::TRANSFER});
    transferCommandList->Begin();
    transferCommandList->WriteBuffer(storageBuffer, positions.data(),
                                     sizeof(vec3) * positions.size());
    transferCommandList->End();
    renderDevice->Submit(transferCommandList);
    renderDevice->WaitGraphicsSubmissionSemaphore();

    const auto swapchainImageCount = renderDevice->GetSwapchainImageCount();

    // Descriptor bindings.
    DescriptorLayoutHandle descriptorLayout;
    std::vector<DescriptorSetHandle> descriptorSets(swapchainImageCount);

    BufferDescriptorItem bufferBinding;
    bufferBinding.buffer = storageBuffer;
    bufferBinding.size = vertexBufferDesc.size;
    bufferBinding.descriptorItem.type = vk::DescriptorType::eStorageBuffer;
    bufferBinding.descriptorItem.shaderStageFlags = vk::ShaderStageFlagBits::eVertex;

    DescriptorLayoutDesc dsLayoutDesc;
    dsLayoutDesc.bufferDescriptors.push_back(bufferBinding);
    dsLayoutDesc.poolCountMultiplier = swapchainImageCount;
    descriptorLayout = renderDevice->CreateDescriptorLayout(dsLayoutDesc);

    DescriptorSetDesc dsSetDesc;
    dsSetDesc.layout = descriptorLayout;
    for (int i = 0; i < swapchainImageCount; i++)
    {
        descriptorSets[i] = renderDevice->CreateDescriptorSet(dsSetDesc);
    }

    // Graphics pipeline setup.
    RenderPassHandle renderPass;
    std::vector<FramebufferHandle> swapchainFramebuffers(swapchainImageCount);
    GraphicsPipelineHandle pipeline;

    RenderPassDesc rpDesc;
    rpDesc.clearColor = true;
    rpDesc.clearDepth = false;
    rpDesc.hasDepth = false;
    rpDesc.flags = RenderPassFlags::First | RenderPassFlags::Last;
    rpDesc.colorFormat = vk::Format::eB8G8R8A8Unorm;
    renderPass = renderDevice->CreateRenderPass(rpDesc);

    const auto& swapchainImages = renderDevice->GetSwapchainImages();
    FramebufferDesc fbDesc;
    fbDesc.renderPass = renderPass;
    fbDesc.width = FRAMEBUFFER_WIDTH;
    fbDesc.height = FRAMEBUFFER_HEIGHT;
    for (int i = 0; i < swapchainImageCount; i++)
    {
        fbDesc.attachments.clear();
        fbDesc.attachments.push_back(swapchainImages[i]);
        swapchainFramebuffers[i] = renderDevice->CreateFramebuffer(fbDesc);
    }

    GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.descriptorSetLayout = descriptorLayout->descriptorSetLayout;
    pipelineDesc.width = FRAMEBUFFER_WIDTH;
    pipelineDesc.height = FRAMEBUFFER_HEIGHT;
    pipelineDesc.useBlending = false;
    pipelineDesc.useDepth = false;
    pipelineDesc.useDynamicScissorState = false;
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertexShader = vs;
    pipelineDesc.fragmentShader = fs;
    pipeline = renderDevice->CreateGraphicsPipeline(pipelineDesc);

    // Command buffers setup.
    std::vector<CommandListHandle> commandLists(swapchainImageCount);
    for (int i = 0; i < swapchainImageCount; i++)
    {
        commandLists[i] = renderDevice->CreateCommandList({});
    }

    DrawArguments drawArgs;
    drawArgs.vertexCount = 3;
    drawArgs.instanceCount = 1;

    GraphicsState graphicsState;
    graphicsState.renderPass = renderPass;
    graphicsState.pipeline = pipeline;

    while (!glfwWindowShouldClose(g_Window))
    {
        renderDevice->SwapchainAcquireNextImage();

        auto imageIndex = renderDevice->GetCurrentSwapchainImageIndex();
        auto& commandList = commandLists[imageIndex];

        graphicsState.descriptorSet = descriptorSets[imageIndex];
        graphicsState.frameBuffer = swapchainFramebuffers[imageIndex];

        commandList->Begin();
        commandList->SetGraphicsState(graphicsState);
        commandList->Draw(drawArgs);
        commandList->End();

        renderDevice->Submit(commandList);
        renderDevice->Present();

        glfwPollEvents();
    }

    renderDevice->WaitGraphicsSubmissionSemaphore();

    WindowDestroy();

    LOG_INFO(u32(-1));

    return 0;
}
