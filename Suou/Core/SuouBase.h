#pragma once

#define SUOU_NON_COPYABLE(cls) \
    cls(const cls&) = delete;  \
    cls& operator=(const cls&) = delete

#define SUOU_NON_MOVEABLE(cls)    \
    cls(cls&&) noexcept = delete; \
    cls& operator=(cls&&) noexcept = delete
