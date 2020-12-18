#include "MixedSet.h"
#include "vec3.h"
#include "Test.h"

#include <functional>
#include <string>

namespace
{
	// Generate a random vec3 of which coordinates
	// fall into [a, b] U [-a, -b]
	vec3 RandomVec3Helper(int a, int b)
	{
		return vec3{
			RandSign() * RandInt(a, b),
			RandSign() * RandInt(a, b),
			RandSign() * RandInt(a, b),
		};
	}

	// Generate a random vec3, which has probability
	// p for being in the sphere with radius r, and
	// (1-p) for being outside of it, respectively
	// considering 1-norm
	// 0 <= d <= 1
	// 0 <= r < INT_MAX / 10^3
	vec3 RandomVec3(int r, float p)
	{
		constexpr int m = 1000;
		if (Percent(p))
		{
			return RandomVec3Helper(0, r - 1);
		}
		else
		{
			return RandomVec3Helper(r + 1, r * m);
		}
	}
}

template<
	size_t HalfWidth,
	size_t BlockSize,
	unsigned InnerPointsPercentage
>
double Benchmark(size_t vecNo, size_t threadNo, float maxLoadFactor)
{
	constexpr static int width = 2 * HalfWidth;
	constexpr static float p = InnerPointsPercentage / 100.f;
	MixedSet<vec3, Vec3Linearizer<HalfWidth>, BlockSize> set;
	set.max_load_factor(maxLoadFactor);
	std::vector<std::thread> threads(threadNo);

	auto start = std::chrono::system_clock::now();

	for (auto& thread : threads)
	{
		thread = std::thread([&set, N = vecNo / threadNo]() mutable
		{
			while (N--) set.insert(RandomVec3(HalfWidth, p));
		});
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	auto end = std::chrono::system_clock::now();
	double elapsed_seconds = std::chrono::duration_cast<
		std::chrono::duration<double>>(end - start).count();
	return elapsed_seconds;
}

template<
	size_t HalfWidth,
	size_t BlockSize,
	unsigned InnerPointsPercentage
>
void RunTests(size_t vecNo, size_t minThreads, size_t maxThreads, float maxLoadFactor = 512.)
{
	for (size_t i = minThreads; i <= maxThreads; ++i)
	{
		size_t inner = vecNo * InnerPointsPercentage / 100;
		size_t outer = vecNo - inner;
		std::cout
			<< "====================================\n"
			<< i << " threads\n"
			<< BlockSize << " block size\n"
			<< inner << " inner points approx.\n"
			<< outer << " outer points approx.\n"
			<< maxLoadFactor << " max load factor.\n"
			<< Benchmark<HalfWidth, BlockSize, InnerPointsPercentage>(vecNo, i, maxLoadFactor)
			<< " seconds" << std::endl;
	}
}

template<
	size_t HalfWidth,
	unsigned InnerPointsPercentage,
	size_t BlockSize
>
void RunForAllThreads(size_t vecNo, float maxLoadFactor = 512.)
{
	size_t threadNo = 2 * std::thread::hardware_concurrency();
	RunTests<HalfWidth, BlockSize, InnerPointsPercentage>(vecNo, threadNo, threadNo, maxLoadFactor);
}
void PerformanceTest()
{
	constexpr size_t vecNo = 100'000'000;

	auto title = [](std::string_view str)
	{
		std::cout << "\n\n\n =======" << str << " ======= \n";
	};

	title("Testing for block size");
	RunForAllThreads<600, 0, 2048>(vecNo);
	RunForAllThreads<600, 0, 1024>(vecNo);
	RunForAllThreads<600, 0, 512> (vecNo);
	RunForAllThreads<600, 0, 256> (vecNo);
	RunForAllThreads<600, 0, 128> (vecNo);
	RunForAllThreads<600, 0, 64>  (vecNo);
	RunForAllThreads<600, 0, 32>  (vecNo);
	RunForAllThreads<600, 0, 16>  (vecNo);

	title("Testing for max load factor");
	RunForAllThreads<600, 0, 256>(vecNo, 128.);
	RunForAllThreads<600, 0, 256>(vecNo, 256.);
	RunForAllThreads<600, 0, 256>(vecNo, 512.);
	RunForAllThreads<600, 0, 256>(vecNo, 768.);
	RunForAllThreads<600, 0, 256>(vecNo, 1024.);
	RunForAllThreads<600, 0, 256>(vecNo, 1280.);
	RunForAllThreads<600, 0, 256>(vecNo, 1536.);
	RunForAllThreads<600, 0, 256>(vecNo, 2048.);
	RunForAllThreads<600, 0, 256>(vecNo, 4096.);

	size_t threadNo = std::thread::hardware_concurrency();
	title("Running with multiple threads, from 1 to " + std::to_string(threadNo));
	RunTests<600, 256, 0>(vecNo, 1, size_t(threadNo * 2));

	title("Testing for different point distributions");
	RunForAllThreads<600,   0, 256>(vecNo);
	RunForAllThreads<600,  10, 256>(vecNo);
	RunForAllThreads<600,  20, 256>(vecNo);
	RunForAllThreads<600,  30, 256>(vecNo);
	RunForAllThreads<600,  40, 256>(vecNo);
	RunForAllThreads<600,  50, 256>(vecNo);
	RunForAllThreads<600,  60, 256>(vecNo);
	RunForAllThreads<600,  70, 256>(vecNo);
	RunForAllThreads<600,  80, 256>(vecNo);
	RunForAllThreads<600,  90, 256>(vecNo);
	RunForAllThreads<600, 100, 256>(vecNo);
}