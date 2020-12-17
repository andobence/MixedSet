#include "BitVectorSet.h"
#include "HashSet.h"
#include "MixedSet.h"
#include <optional>
#include <future>
#include <iostream>
#include <random>
#include "catch.hpp"

namespace
{
#ifdef _DEBUG
	constexpr bool c_debug = true;
#else
	constexpr bool c_debug = false;
#endif
	constexpr size_t c_testAdders = 5;
	constexpr size_t c_testErasers = 5;
	constexpr size_t c_testSize = 256;
	constexpr size_t c_testAdds = 1024ull * (c_debug ? 64 : 1024);
	constexpr size_t c_testErases = 4ull * 1024 * (c_debug ? 64 : 1024);

	struct IdentityTransform
	{
		template<typename T>
		auto operator()(T x)
		{
			return x;
		}
	};

	struct TestLinearizer
	{
		static constexpr size_t size = 100;

		std::optional<int> operator()(int x)
		{
			if (x < 0 || x >= 100)
				return std::nullopt;

			return x;
		}
	};

	struct TestBitVector : BitVectorSet
	{
		TestBitVector() : BitVectorSet(c_testSize)
		{
		}
	};
}

template<typename Transform, typename Set>
std::vector<long long> Inserter(Set& set)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, c_testSize - 1);
	std::vector<long long> changes;
	changes.resize(c_testSize);
	Transform trans;

	for (size_t i = 0; i < c_testAdds; i++)
	{
		auto index = dist(gen);
		if (set.insert(trans(index)))
		{
			changes[index]++;
		}
	}

	return changes;
}

template<typename Transform, typename Set>
std::vector<long long> Eraser(Set& set)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, c_testSize - 1);
	std::vector<long long> changes;
	changes.resize(c_testSize);
	Transform trans;

	for (size_t i = 0; i < c_testErases; i++)
	{
		auto index = dist(gen);
		if (set.erase(trans(index)))
		{
			changes[index]--;
		}
	}

	return changes;
}

template<typename Transform, typename Set>
void TestSetInsertErase(Set set)
{
	std::vector<std::future<std::vector<long long>>> threads;

	for (size_t i = 0; i < c_testAdders; i++)
	{
		threads.push_back(std::async(std::launch::async, Inserter<Transform, Set>, std::ref(set)));
	}

	for (size_t i = 0; i < c_testErasers; i++)
	{
		threads.push_back(std::async(std::launch::async, Eraser<Transform, Set>, std::ref(set)));
	}

	std::vector<long long> changes;
	changes.resize(c_testSize);

	for (auto& thread : threads)
	{
		auto vec = thread.get();

		for (size_t i = 0; i < c_testSize; i++)
		{
			changes[i] += vec[i];
		}
	}

	size_t less0 = 0;
	size_t equal0 = 0;
	size_t above0 = 0;

	for (size_t i = 0; i < c_testSize; i++)
	{
		if (changes[i] < 0)
			less0++;
		else if (changes[i] > 0)
			above0++;
		else
			equal0++;
	}

	CHECK(less0 >= 0);
}

TEMPLATE_TEST_CASE("Parallel insert and erase", "[set][template]", TestBitVector, (HashSet<int>), (MixedSet<int, TestLinearizer>))
{
	TestSetInsertErase<IdentityTransform>(TestType{});
}

TEMPLATE_TEST_CASE("Basics", "[set][template]", TestBitVector, (HashSet<int>), (MixedSet<int, TestLinearizer>))
{
	TestType set;
	
	for (int i = 0; i < 50; i++)
	{
		CAPTURE(i);
		REQUIRE_FALSE(set.contains(i));
	}

	REQUIRE(set.insert(42));
	REQUIRE(set.contains(42));

	for (int i = 0; i < 50; i++)
	{
		if (i != 42)
		{
			CAPTURE(i);
			REQUIRE_FALSE(set.contains(i));
		}
	}

	REQUIRE_FALSE(set.insert(42));
	REQUIRE_FALSE(set.erase(16));
	REQUIRE_FALSE(set.erase(0));
	REQUIRE_FALSE(set.erase(1234567890));
	REQUIRE(set.contains(42));

	REQUIRE(set.erase(42));
	REQUIRE_FALSE(set.contains(42));

	REQUIRE_FALSE(set.contains(0));
	REQUIRE(set.insert(0));
	REQUIRE_FALSE(set.insert(0));
	REQUIRE(set.contains(0));
	REQUIRE(set.erase(0));
	REQUIRE_FALSE(set.erase(0));
	REQUIRE_FALSE(set.contains(0));
}
