#pragma once

#define Suoh_NON_COPYABLE(cls) \
    cls(const cls&) = delete;  \
    cls& operator=(const cls&) = delete

#define Suoh_NON_MOVEABLE(cls)    \
    cls(cls&&) noexcept = delete; \
    cls& operator=(cls&&) noexcept = delete
