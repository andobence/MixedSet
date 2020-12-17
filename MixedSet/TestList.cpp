#include "List.h"
#include "Test.h"

#include <iostream>
#include <type_traits>
#include <set>
#include <vector>
#include <fstream>
#include <execution>
#include "catch.hpp"

namespace
{
	auto IntList()
	{
		return List<int>();
	}

	constexpr auto testFname = "test.txt";
	constexpr std::size_t N{ 10000 };
	constexpr std::size_t M{ 200000 };

	void SaveRandomSequence(const std::vector<int>& container)
	{
		std::ofstream f{ testFname };
		f << container.size() << " ";
		for (const auto& e : container) f << e << " ";
	}
	auto GetRandomSequence(int from, int to)
	{
		std::ifstream f{ testFname };
		std::vector<int> ret;
		int e{ 0 };
		f >> e;
		ret.reserve(e);
		while (f >> e) ret.push_back(e);
		if (ret.empty())
			for (int i = 0; i < N; ++i) ret.push_back(RandInt(from, to));
		return ret;
	}
}

TEST_CASE("Sequential add", "[list]")
{
	DYNAMIC_SECTION("Inserting numbers from 1 to " << N)
	{
		auto list = IntList();

		for (int i = 1; i <= N; ++i)
		{
			list.insert(i);
		}

		for (int i = 1; i <= N; ++i)
		{
			REQUIRE(list.contains(i));
		}
	}

	DYNAMIC_SECTION("Inserting " << N << " uniformly random integers between 1 and " << M)
	{
		auto randomSequence = GetRandomSequence(1, M);

		auto set = std::set<int>();
		auto list = IntList();

		int i = 0;
		for (const auto& random : randomSequence)
		{
			++i;

			auto [_, insertedIntoSet] = set.insert(random);
			bool insertedIntoList = list.insert(random);

			if (insertedIntoSet != insertedIntoList)
			{
				SaveRandomSequence(randomSequence);

				INFO("i = " << i);
				REQUIRE(insertedIntoSet != insertedIntoList);
			}
		}
	}

	DYNAMIC_SECTION("Inserting numbers from " << N << " to 1")
	{
		auto list = IntList();

		for (int i = N; i >= 1; --i)
		{
			list.insert(i);
		}

		for (int i = N; i >= 1; --i)
		{
			REQUIRE(list.contains(i));
		}
	}
}

TEST_CASE("Parallel add", "[list]")
{
	DYNAMIC_SECTION("Inserting " << N << " uniformly random numbers from 1 to " << M << " parallelly")
	{
		auto randomSequence = GetRandomSequence(1, M);

		auto list = IntList();
		std::for_each(std::execution::par,
			randomSequence.begin(),
			randomSequence.end(),
			[&list](int random)
			{
				list.insert(random);
			});

		auto set = std::set(randomSequence.begin(), randomSequence.end());

		for (auto random : randomSequence)
		{
			REQUIRE((set.count(random) > 0) == list.contains(random));
		}
	}
}

TEST_CASE("Sequential erase", "[list]")
{
	auto testErase = [](const std::vector<int>& sequence)
	{
		auto list = IntList();

		for (auto i : sequence)
		{
			list.insert(i);
		}

		for (auto i : sequence)
		{
			if (!list.erase(i))
			{
				SaveRandomSequence(sequence);
				FAIL("Set did not contain " << i);
			}
		}

		for (auto i : sequence)
		{
			if (list.contains(i))
			{
				SaveRandomSequence(sequence);
				FAIL("Set still contains " << i);
			}
		}
	};

	auto ints = std::vector<int>(N);
	std::iota(std::begin(ints), std::end(ints), 1);
	DYNAMIC_SECTION("Inserting numbers from 1 to " << N << " and then erasing them in direct order")
	{
		testErase(ints);
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(std::begin(ints), std::end(ints), gen);
	DYNAMIC_SECTION("Inserting numbers from 1 to " << N << " and then erasing them in random order")
	{
		testErase(ints);
	}
}
