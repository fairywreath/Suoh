#pragma once

#include "VulkanCommon.h"

class Buffer : public GPUResource
{
public:
    Buffer(VulkanDevice* device, const BufferDesc& createDesc);
    virtual ~Buffer();

    const BufferDesc& GetDesc() const
    {
        return m_Desc;
    }

    vk::Buffer GetVulkanHandle() const
    {
        return m_VulkanBuffer;
    }

    VmaAllocation GetAlocation() const
    {
        return m_VmaAllocation;
    }

    u32 GetSize() const
    {
        return m_Size;
    }

    vk::BufferUsageFlags GetBufferUsageFlags() const
    {
        return m_BufferUsageFlags;
    }

    ResourceUsageType GetResourceUsage() const
    {
        return m_ResourceUsage;
    }

    bool IsReady() const
    {
        return m_Ready;
    }

    u32 GetGlobalOffset() const
    {
        return m_GlobalOffset;
    }

    void* GetMappedData() const
    {
        return m_MappedData;
    }

    Buffer* GetParentBuffer() const
    {
        return m_ParentBuffer;
    }

private:
    VulkanDevice* m_Device;
    BufferDesc m_Desc;

    vk::Buffer m_VulkanBuffer;
    VmaAllocation m_VmaAllocation;

    u32 m_Size{0};
    vk::BufferUsageFlags m_BufferUsageFlags{0};
    ResourceUsageType m_ResourceUsage{ResourceUsageType::Immutable};

    u32 m_GlobalOffset{0};

    bool m_Ready{true};
    void* m_MappedData{nullptr};

    std::string m_Name;

    // For shared buffers.
    BufferRef m_ParentBuffer;

    friend class VulkanDevice;
};

struct MapBufferParameters
{
    MapBufferParameters() = default;

    // For implicit conversion.
    MapBufferParameters(Buffer* _buffer) : buffer(_buffer){};

    Buffer* buffer{nullptr};
    u32 offset{0};
    u32 size{0};
};
