#pragma once

#include "CoreTypes.h"

#define SUOHRHI_ENUM_CLASS_FLAG_OPERATORS(T)                                                                                               \
    inline T operator|(T a, T b)                                                                                                           \
    {                                                                                                                                      \
        return T(u32(a) | u32(b));                                                                                                         \
    }                                                                                                                                      \
    inline T operator&(T a, T b)                                                                                                           \
    {                                                                                                                                      \
        return T(u32(a) & u32(b));                                                                                                         \
    }                                                                                                                                      \
    inline T operator~(T a)                                                                                                                \
    {                                                                                                                                      \
        return T(~u32(a));                                                                                                                 \
    }                                                                                                                                      \
    inline bool operator!(T a)                                                                                                             \
    {                                                                                                                                      \
        return u32(a) == 0;                                                                                                                \
    }                                                                                                                                      \
    inline bool operator==(T a, u32 b)                                                                                                     \
    {                                                                                                                                      \
        return u32(a) == b;                                                                                                                \
    }                                                                                                                                      \
    inline bool operator!=(T a, u32 b)                                                                                                     \
    {                                                                                                                                      \
        return u32(a) != b;                                                                                                                \
    }