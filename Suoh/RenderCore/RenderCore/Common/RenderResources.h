#pragma once

#include <CoreTypes.h>

namespace RenderCore
{

class IResource
{
public:
    NON_COPYABLE(IResource);
    NON_MOVEABLE(IResource);

    virtual u64 AddRef() = 0;
    virtual u64 Release() = 0;

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

} // namespace RenderCore
