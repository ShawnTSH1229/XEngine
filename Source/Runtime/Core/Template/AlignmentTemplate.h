#pragma once
#include <type_traits>
#include "Runtime/HAL/PlatformTypes.h"
template <typename T>
inline constexpr T Align(T Val, uint64 Alignment)
{
	static_assert(std::is_integral_v<T> || std::is_pointer_v<T>);
	return (T)(((uint64)Val + Alignment - 1) & ~(Alignment - 1));
}

