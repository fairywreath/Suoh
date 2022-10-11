#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "RHICommon.h"

namespace SuohRHI
{

template <typename T, u32 _max_elements> struct static_vector : private std::array<T, _max_elements>
{
    using base = std::array<T, _max_elements>;
    enum
    {
        max_elements = _max_elements
    };

    using typename base::const_iterator;
    using typename base::const_pointer;
    using typename base::const_reference;
    using typename base::difference_type;
    using typename base::iterator;
    using typename base::pointer;
    using typename base::reference;
    using typename base::size_type;
    using typename base::value_type;

    static_vector() : base(), mCurrentSize(0)
    {
    }

    static_vector(size_t size) : base(), mCurrentSize(size)
    {
        assert(size <= max_elements);
    }

    static_vector(const static_vector& other) = default;

    static_vector(std::initializer_list<T> il) : mCurrentSize(0)
    {
        for (auto i : il)
            push_back(i);
    }

    using base::at;

    reference operator[](size_type pos)
    {
        assert(pos < mCurrentSize);
        return base::operator[](pos);
    }

    const_reference operator[](size_type pos) const
    {
        assert(pos < mCurrentSize);
        return base::operator[](pos);
    }

    using base::front;

    reference back() noexcept
    {
        auto tmp = end();
        --tmp;
        return *tmp;
    }
    const_reference back() const noexcept
    {
        auto tmp = cend();
        --tmp;
        return *tmp;
    }

    using base::begin;
    using base::cbegin;
    using base::data;

    iterator end() noexcept
    {
        return iterator(begin()) + mCurrentSize;
    }
    const_iterator end() const noexcept
    {
        return cend();
    }
    const_iterator cend() const noexcept
    {
        return const_iterator(cbegin()) + mCurrentSize;
    }

    bool empty() const noexcept
    {
        return mCurrentSize == 0;
    }
    size_t size() const noexcept
    {
        return mCurrentSize;
    }
    constexpr size_t max_size() const noexcept
    {
        return max_elements;
    }

    void fill(const T& value) noexcept
    {
        base::fill(value);
        mCurrentSize = max_elements;
    }

    void swap(static_vector& other) noexcept
    {
        base::swap(*this);
        std::swap(mCurrentSize, other.mCurrentSize);
    }

    void push_back(const T& value) noexcept
    {
        assert(mCurrentSize < max_elements);
        *(data() + mCurrentSize) = value;
        mCurrentSize++;
    }

    void push_back(T&& value) noexcept
    {
        assert(mCurrentSize < max_elements);
        *(data() + mCurrentSize) = std::move(value);
        mCurrentSize++;
    }

    void pop_back() noexcept
    {
        assert(mCurrentSize > 0);
        mCurrentSize--;
    }

    void resize(size_type new_size) noexcept
    {
        assert(new_size <= max_elements);

        if (mCurrentSize > new_size)
        {
            for (size_type i = new_size; i < mCurrentSize; i++)
                (data() + i)->~T();
        }
        else
        {
            for (size_type i = mCurrentSize; i < new_size; i++)
                new (data() + i) T();
        }

        mCurrentSize = new_size;
    }

    reference emplace_back() noexcept
    {
        resize(mCurrentSize + 1);
        return back();
    }

private:
    size_type mCurrentSize = 0;
};

} // namespace SuohRHI
