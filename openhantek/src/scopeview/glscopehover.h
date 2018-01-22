#pragma once

#include <type_traits>

enum class EdgePositionFlags : int {
    None = 0,
    Left = 1,
    Right = 2,
    Top = 4,
    Bottom = 8,
    Middle = Left | Right | Top | Bottom
};

using T = std::underlying_type<EdgePositionFlags>::type;
inline EdgePositionFlags operator|(EdgePositionFlags lhs, EdgePositionFlags rhs) {
    return (EdgePositionFlags)(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline EdgePositionFlags &operator|=(EdgePositionFlags &lhs, EdgePositionFlags rhs) {
    lhs = (EdgePositionFlags)(static_cast<T>(lhs) | static_cast<T>(rhs));
    return lhs;
}
inline bool operator/(EdgePositionFlags lhs, EdgePositionFlags rhs) {
    return (EdgePositionFlags)(static_cast<T>(lhs) & static_cast<T>(rhs)) == rhs;
}
