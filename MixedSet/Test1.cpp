#include "BitVectorSet.h"
#include <future>
#include <iostream>
#include <random>

namespace
{
	constexpr size_t c_testAdders = 5;
	constexpr size_t c_testErasers = 5;
	constexpr size_t c_testSize = 256;
	constexpr size_t c_testAdds = 256 * 1024 * 1024;
	constexpr size_t c_testErases = 4ull * 256 * 1024 * 1024;
}

std::vector<long long> Adder(BitVectorSet& set)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, c_testSize - 1);
	std::vector<long long> changes;
	changes.resize(c_testSize);

	for (size_t i = 0; i < c_testAdds; i++)
	{
		auto index = dist(gen);
		if (set.insert(index))
		{
			changes[index]++;
		}
	}

	return changes;
}

std::vector<long long> Eraser(BitVectorSet& set)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, c_testSize - 1);
	std::vector<long long> changes;
	changes.resize(c_testSize);

	for (size_t i = 0; i < c_testErases; i++)
	{
		auto index = dist(gen);
		if (set.erase(index))
		{
			changes[index]--;
		}
	}

	return changes;
}

bool Test1()
{
	BitVectorSet set(c_testSize);

	std::vector<std::future<std::vector<long long>>> threads;

	for (size_t i = 0; i < c_testAdders; i++)
	{
		threads.push_back(std::async(std::launch::async, Adder, std::ref(set)));
	}

	for (size_t i = 0; i < c_testErasers; i++)
	{
		threads.push_back(std::async(std::launch::async, Eraser, std::ref(set)));
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

	std::cout << "<0: " << less0 << std::endl;
	std::cout << "=0: " << equal0 << std::endl;
	std::cout << ">0: " << above0 << std::endl;

	return less0 > 0;
}