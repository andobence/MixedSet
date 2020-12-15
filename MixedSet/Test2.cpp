#include "MixedSet.h"
#include <iostream>
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

struct Vec3Linearizer
{
	static constexpr size_t halfwidth = 64;
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

void Test2()
{
	MixedSet<vec3, Vec3Linearizer> set;

	for (int i = 0; i < 1024*1024*2; i++)
	{
		set.insert({ i, 0, 0 });
	}

#define TEST(cmd) std::cout << #cmd << '\t' << (cmd) << std::endl;

	TEST(set.contains({ 10, 0, 0 }));
	TEST(set.contains({ 11, 0, 0 }));
	TEST(set.contains({ 1022, 0, 0 }));
	TEST(set.contains({ 1023, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.erase({ 11, 0, 0 }));
	TEST(set.erase({ 1023, 0, 0 }));
	TEST(set.erase({ 11, 0, 0 }));
	TEST(set.contains({ 10, 0, 0 }));
	TEST(set.contains({ 11, 0, 0 }));
	TEST(set.contains({ 1022, 0, 0 }));
	TEST(set.contains({ 1023, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.insert({ 1024, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.erase({ 1024, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
}
