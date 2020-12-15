#include "List.h"

#include <iostream>
#include <type_traits>
#include <random>
#include <set>
#include <vector>
#include <fstream>
#include <execution>

namespace
{
	auto IntList()
	{
		return List<int>();
	}
	auto RandInt(int from, int to)
	{
		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(from, to);
		return distrib(gen);
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

void TestSequentialAdd()
{

	std::cout << "Inserting numbers from 1 to N\n";
	{
		auto list = IntList();

		for (int i = 1; i <= N; ++i)
		{
			list.insert(i);
		}

		assert(list.size() == N);

		for (int i = 1; i <= N; ++i)
		{
			assert(list.contains(i));
		}
	}

	std::cout << "Inserting N uniformly random integers between 1 and M\n";
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
				assert(false && i);
				return;
			}
		}

		assert(set.size() == list.size());
	}

	std::cout << "Inserting numbers from N to 1\n";
	{
		auto list = IntList();

		for (int i = N; i >= 1; --i)
		{
			list.insert(i);
		}

		assert(list.size() == N);

		for (int i = N; i >= 1; --i)
		{
			assert(list.contains(i));
		}
	}
}

void TestParallelAdd()
{
	std::cout << "Inserting N uniformly random numbers from 1 to M parallelly\n";
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

		assert(list.size() == set.size());
		for (auto random : randomSequence)
		{
			assert((set.count(random) > 0) == list.contains(random));
		}
	}
}

void TestSequentialErase()
{
	auto testErase = [](const std::vector<int>& sequence)
	{
		auto list = IntList();

		for (auto i : sequence)
		{
			list.insert(i);
		}
		assert(list.size() == N);

		for (auto i : sequence)
		{
			if (!list.erase(i))
			{
				SaveRandomSequence(sequence);
				assert(false && i);
			}
		}
		assert(list.size() == 0);

		for (auto i : sequence)
		{
			if (list.contains(i))
			{
				SaveRandomSequence(sequence);
				assert(false && i);
			}
		}
	};

	auto ints = std::vector<int>(N);
	std::iota(std::begin(ints), std::end(ints), 1);
	std::cout << "Inserting numbers from 1 to N and then erasing them in direct order\n";
	testErase(ints);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(std::begin(ints), std::end(ints), gen);
	std::cout << "Inserting numbers from 1 to N and then erasing them in random order\n";
	testErase(ints);
}

void TestList()
{
	TestSequentialAdd();
	TestParallelAdd();
	TestSequentialErase();
}