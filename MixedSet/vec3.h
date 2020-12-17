#pragma once

#include "MixedSet.h"
#include <optional>

struct vec3
{
	int x, y, z;

	auto asTuple() const
	{
		return std::make_tuple(x, y, z);
	}

	bool operator==(const vec3& rhs) const
	{
		return asTuple() == rhs.asTuple();
	}

	bool operator<(const vec3& rhs) const
	{
		return asTuple() < rhs.asTuple();
	}
};

namespace std {
	template <> struct hash<vec3>
	{
		size_t operator()(const vec3& value) const
		{
			return hash<int>()(value.x) ^ hash<int>()(value.y) ^ hash<int>()(value.z);
		}
	};
}

template<size_t halfwidth = 64>
struct Vec3Linearizer
{
	static constexpr size_t size = 8 * halfwidth * halfwidth * halfwidth;

	std::optional<size_t> operator()(vec3 value)
	{
		// (halfwidth = 2) => ... -2 (-1 0 1 2) 3 ... -> ... -1 (0, 1, 2, 3) 4 ...
		value.x += halfwidth - 1;
		value.y += halfwidth - 1;
		value.z += halfwidth - 1;

		auto isOutOfRange = [](int v)
		{
			return v < 0 || v >= 2 * halfwidth;
		};

		if (isOutOfRange(value.x) || isOutOfRange(value.y) || isOutOfRange(value.z))
			return std::nullopt;

		return value.x + 2 * halfwidth * value.y + 4 * halfwidth * halfwidth * value.z;
	}
};
