#pragma once

#include "CoreTypes.h"

#include <atomic>
#include <type_traits>
#include <variant>

namespace SuohRHI
{

using ObjectType = u32;

namespace ObjectTypes
{

constexpr ObjectType Vk_Device = 0x00001001;
constexpr ObjectType Vk_PhysicalDevice = 0x00001002;
constexpr ObjectType Vk_Instance = 0x00001003;
constexpr ObjectType Vk_Queue = 0x00001004;
constexpr ObjectType Vk_DeviceMemory = 0x00001005;
constexpr ObjectType Vk_Buffer = 0x00001006;
constexpr ObjectType Vk_Image = 0x00001007;
constexpr ObjectType Vk_ImageView = 0x00001008;
constexpr ObjectType Vk_ShaderModule = 0x00001009;
constexpr ObjectType Vk_Sampler = 0x0000100a;
constexpr ObjectType Vk_RenderPass = 0x0000100b;
constexpr ObjectType Vk_FrameBuffer = 0x0000100c;
constexpr ObjectType Vk_DescriptorSetLayout = 0x0000100d;
constexpr ObjectType Vk_DescriptorSet = 0x0000100e;
constexpr ObjectType Vk_DescriptorPool = 0x0000100f;
constexpr ObjectType Vk_PipelineLayout = 0x0000100a;
constexpr ObjectType Vk_Pipeline = 0x00001011;
constexpr ObjectType Vk_CommandBuffer = 0x00001012;
constexpr ObjectType Vk_Semaphore = 0x00001013;
constexpr ObjectType Vk_Fence = 0x00001014;

constexpr ObjectType Vk_Surface_KHR = 0x00001101;
constexpr ObjectType Vk_Swapchain_KHR = 0x00001102;

} // namespace ObjectTypes

struct Object
{
    std::variant<u64, void*> handle;

    explicit Object(u64 i) : handle(i){};
    explicit Object(void* ptr) : handle(ptr){};
};

class IResource
{
public:
    NON_COPYABLE(IResource);
    NON_MOVEABLE(IResource);

    virtual u64 AddRef() = 0;
    virtual u64 Release() = 0;

    virtual Object GetNativeObject(ObjectType objectType)
    {
        (void)objectType;
        return Object(nullptr);
    }

protected:
    IResource() = default;
    virtual ~IResource() = default;
};

/*
 * Based on ComPtr.
 */
template <typename T> class RefCountPtr
{
public:
    using InterfaceType = T;

    RefCountPtr() noexcept : mPtr(nullptr)
    {
    }

    RefCountPtr(std::nullptr_t) noexcept : mPtr(nullptr)
    {
    }

    template <class U> RefCountPtr(U* otherPtr) noexcept : mPtr(otherPtr)
    {
        InternalAddRef();
    }

    RefCountPtr(const RefCountPtr& other) noexcept : mPtr(other.mPtr)
    {
        InternalAddRef();
    }

    template <class U>
    RefCountPtr(const RefCountPtr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
        : mPtr(other.mPtr)
    {
        InternalAddRef();
    }

    RefCountPtr(RefCountPtr&& other) noexcept : mPtr(nullptr)
    {
        if (this != reinterpret_cast<RefCountPtr*>(&reinterpret_cast<u8&>(other)))
        {
            Swap(other);
        }
    }

    template <class U>
    RefCountPtr(RefCountPtr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept
        : mPtr(other.ptr)
    {
        other.ptr = nullptr;
    }

    ~RefCountPtr() noexcept
    {
        InternalRelease();
    }

    RefCountPtr& operator=(std::nullptr_t) noexcept
    {
        InternalRelease();
        return *this;
    }

    RefCountPtr& operator=(T* other) noexcept
    {
        if (mPtr != other)
        {
            RefCountPtr(other).Swap(*this);
        }

        return *this;
    }

    template <typename U> RefCountPtr& operator=(U* other) noexcept
    {
        RefCountPtr(other).Swap(*this);
        return *this;
    }

    RefCountPtr& operator=(const RefCountPtr& other) noexcept
    {
        if (mPtr != other.mPtr)
        {
            RefCountPtr(other).Swap(*this);
        }
        return *this;
    }

    template <class U> RefCountPtr& operator=(const RefCountPtr<U>& other) noexcept
    {
        RefCountPtr(other).Swap(*this);
        return *this;
    }

    RefCountPtr& operator=(RefCountPtr&& other) noexcept
    {
        RefCountPtr(static_cast<RefCountPtr&&>(other)).Swap(*this);
        return *this;
    }

    template <class U> RefCountPtr& operator=(RefCountPtr<U>&& other) noexcept
    {
        RefCountPtr(static_cast<RefCountPtr<U>&&>(other)).Swap(*this);
        return *this;
    }

    void Swap(RefCountPtr&& other) noexcept
    {
        T* tmp = mPtr;
        mPtr = other.mPtr;
        other.mPtr = tmp;
    }

    void Swap(RefCountPtr& other) noexcept
    {
        T* tmp = mPtr;
        mPtr = other.mPtr;
        other.mPtr = tmp;
    }

    [[nodiscard]] T* Get() const noexcept
    {
        return mPtr;
    }

    operator T*() const
    {
        return mPtr;
    }

    InterfaceType* operator->() const noexcept
    {
        return mPtr;
    }

    T** operator&()
    {
        return &mPtr;
    }

    [[nodiscard]] T* const* GetAddressOf() const noexcept
    {
        return &mPtr;
    }

    [[nodiscard]] T** GetAddressOf() noexcept
    {
        return &mPtr;
    }

    [[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
    {
        InternalRelease();
        return &mPtr;
    }

    T* Detach() noexcept
    {
        T* ptr = mPtr;
        mPtr = nullptr;
        return ptr;
    }

    void Attach(InterfaceType* other)
    {
        if (mPtr)
        {
            auto ref = mPtr->Release();
            (void)ref;
        }

        mPtr = other;
    }

    static RefCountPtr<T> Create(T* other)
    {
        RefCountPtr<T> Ptr;
        Ptr.Attach(other);
        return Ptr;
    }

    u64 Reset()
    {
        return InternalRelease();
    }

    template <bool b, typename U = void> struct EnableIf
    {
    };

    template <typename U> struct EnableIf<true, U>
    {
    };

protected:
    InterfaceType* mPtr;
    template <class U> friend class RefCountPtr;

    void InternalAddRef() const noexcept
    {
        if (mPtr)
        {
            mPtr->AddRef();
        }
    }

    u64 InternalRelease() noexcept
    {
        u64 ref = 0;
        InterfaceType* temp = mPtr;

        if (temp)
        {
            mPtr = nullptr;
            ref = temp->Release();
        }

        return ref;
    }
};

template <class T> class RefCountResource : public T
{

public:
    virtual u64 AddRef() override
    {
        return ++mRefCount;
    }

    virtual u64 Release() override
    {
        u64 result = --mRefCount;
        if (result == 0)
        {
            delete this;
        }

        return result;
    }

private:
    std::atomic<u64> mRefCount{1};
};

using ResourceHandle = RefCountPtr<IResource>;

} // namespace SuohRHI
