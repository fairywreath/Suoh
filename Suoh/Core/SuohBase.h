#pragma once

#define SUOH_NON_COPYABLE(cls) \
    cls(const cls&) = delete;  \
    cls& operator=(const cls&) = delete

#define SUOH_NON_MOVEABLE(cls)    \
    cls(cls&&) noexcept = delete; \
    cls& operator=(cls&&) noexcept = delete
