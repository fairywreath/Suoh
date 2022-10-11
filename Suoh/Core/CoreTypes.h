#pragma once

#include <cstdint>
#include <glm/glm.hpp>

using f32 = float;
using f64 = double;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using mat3 = glm::mat3;
using mat4 = glm::mat4;
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

#define NON_COPYABLE(cls)                                                                                                                  \
    cls(const cls&) = delete;                                                                                                              \
    cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls)                                                                                                                  \
    cls(cls&&) noexcept = delete;                                                                                                          \
    cls& operator=(cls&&) noexcept = delete
