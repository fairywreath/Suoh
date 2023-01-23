#pragma once

#include <cassert>
#include <queue>
#include <vector>

#include <Core/Types.h>

/*
 * Generic index-based resource array pool.
 * Resource objects are required to have a default constructor.
 * XXX: Make static instead?
 */
template <typename T> class ResourcePool
{
public:
    ResourcePool() = default;
    ResourcePool(u32 initialCapacity)
    {
        m_Memory.reserve(initialCapacity);
    }
    ~ResourcePool() = default;

    NON_COPYABLE(ResourcePool);
    NON_MOVEABLE(ResourcePool);

    void Reserve(u32 capacity)
    {
        assert(capacity >= m_Memory.size());
        m_Memory.reserve(capacity);
    }

    T* AccessResource(u32 index)
    {
        assert(index < m_Memory.size());

        if (index > m_Memory.size())
        {
            return nullptr;
        }

        return &m_Memory[index];
    }

    const T* AccessResource(u32 index) const
    {
        assert(index < m_Memory.size());

        if (index > m_Memory.size())
        {
            return nullptr;
        }

        return &m_Memory[index];
    }

    u32 AddResource(const T& resource)
    {
        auto index = AcquireNewResourceIndex();
        m_Memory[index] = resource;
        return index;
    }

    u32 AddResource(T&& resource)
    {
        auto index = AcquireNewResourceIndex();
        m_Memory[index] = resource;
        return index;
    }

    void AddResource(const T& resource, u32 index)
    {
        assert(index < m_Memory.size());
        m_Memory[index] = resource;
    }

    void AddResource(T&& resource, u32 index)
    {
        assert(index < m_Memory.size());
        m_Memory[index] = resource;
    }

    void DestroyResource(u32 index)
    {
        assert(index < m_Memory.size());
        // XXX: Check object at index is alive and reset it properly?
        // m_Memory[index] = {};
        m_ReturnedIndices.push(index);
    }

    void DestroyAllResources()
    {
        m_Memory.clear();
        m_ReturnedIndices.clear();
    }

    u32 AcquireNewResourceIndex()
    {
        u32 newIndex;

        if (m_ReturnedIndices.size() > 0)
        {
            newIndex = m_ReturnedIndices.front();
            m_ReturnedIndices.pop();
        }
        else
        {
            newIndex = static_cast<u32>(m_Memory.size());
            m_Memory.emplace_back();
        }

        return newIndex;
    }

    size_t Size() const
    {
        return m_Memory.size() - m_ReturnedIndices.size();
    }

private:
    std::vector<T> m_Memory;
    std::queue<u32> m_ReturnedIndices;
};