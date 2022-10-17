#pragma once

#include "VulkanCommon.h"

namespace SuohRHI
{

namespace Vulkan
{

class VulkanInstance
{
public:
    VulkanInstance();
    ~VulkanInstance();

    NON_COPYABLE(VulkanInstance);
    NON_MOVEABLE(VulkanInstance);

    [[nodiscard]] vk::Instance GetVkInstance() const;

private:
    void InitInstance();
    void DestroyInstance();

private:
    vk::DynamicLoader m_DynamicLoader;

    vk::Instance m_Instance;
    vk::DebugUtilsMessengerEXT m_DebugMessenger;
};

} // namespace Vulkan

} // namespace SuohRHI