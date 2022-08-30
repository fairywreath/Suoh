#pragma once

#include "VKCommon.h"

namespace Suoh
{

/*
    physical and logical device
*/
class VKDevice
{
public:
    VKDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VKDevice();

    void destroy();

    VkPhysicalDevice getPhysical() const;

    // XXX: return reference so we can use callbacks for
    //      object destructions using this device?
    const VkDevice& getLogical() const;

    VkQueue getGraphicsQueue() const;
    u32 getGraphicsFamily() const;

    VkQueue getComputeQueue() const;
    u32 getComputeFamily() const;
    bool computeCapable() const;

    VkQueue getPresentQueue() const;
    u32 getPresentFamily() const;

    u32 getQueueFamilyIndicesCount() const;
    std::vector<u32> getQueueFamilyIndices() const;

    const VkPhysicalDeviceProperties&
    getPhysDeviceProperties() const;

private:
    void initPhysDevice();
    void initLogicalDevice();

private:
    VkInstance mInstance;
    VkSurfaceKHR mSurface;

    VkPhysicalDevice mPhysDevice;
    VkDevice mLogicalDevice;

    VkQueue mGraphicsQueue;
    u32 mGraphicsFamily{};

    VkQueue mComputeQueue;
    u32 mComputeFamily{};
    bool mUseCompute{};

    VkQueue mPresentQueue;
    u32 mPresentFamily{};

    u32 mQueueFamilyIndicesCount{};

    VkPhysicalDeviceProperties mPhysDeviceProperties;
    VkPhysicalDeviceFeatures mPhysDeviceFeatures;
    VkPhysicalDeviceFeatures2 mPhysDeviceFeatures2;
};

} // namespace Suoh
