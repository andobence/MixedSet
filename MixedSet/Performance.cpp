#include "MixedSet.h"
#include "vec3.h"
#include "Test.h"

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
	size_t VecNo,
	size_t ThreadNo,
	size_t HalfWidth,
	size_t BlockSize,
	unsigned InnerPointsPercentage
>
double Benchmark()
{
	constexpr static int width = 2 * HalfWidth;
	constexpr static float p = InnerPointsPercentage / 100.f;
	MixedSet<vec3, Vec3Linearizer<HalfWidth>, BlockSize> set;
	std::array<std::thread, ThreadNo> threads;

	auto start = std::chrono::system_clock::now();

	for (auto& thread : threads)
	{
		thread = std::thread([&set, N = VecNo / ThreadNo]() mutable
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
	size_t MinThreads,
	size_t MaxThreads_,
	size_t VecNo,
	size_t BlockSize,
	unsigned InnerPointsPercentage
>
void RunTests()
{
	constexpr static size_t MaxThreads = MaxThreads_ + 1;
	auto helper = []<size_t... I>(std::index_sequence<I...> _)
	{
		((I >= MinThreads && (
			std::cout
			<< "====================================\n"
			<< I << " threads\n"
			<< BlockSize << " block size\n"
			<< VecNo * InnerPointsPercentage / 100 << " items approx.\n"
			<< "---> " << Benchmark<50'000'000, I, 600, BlockSize, InnerPointsPercentage>() 
				<< " seconds" << std::endl
			)),
			...);
	};

	helper(std::make_index_sequence<MaxThreads>());
}

template<
	size_t VecNo,
	unsigned InnerPointsPercentage,
	size_t BlockSize
>
void RunForAllThreads()
{
	RunTests<8, 8, VecNo, BlockSize, InnerPointsPercentage>();
}

void PerformanceTest()
{
	//RunForAllThreads<50'000'000, 99, 128>();
	//RunForAllThreads<50'000'000, 80, 128>();
	//
	//RunForAllThreads<10'000'000, 60, 128>();
	//RunForAllThreads<10'000'000, 40, 128>();
	//RunForAllThreads<10'000'000, 20, 128>();


	RunForAllThreads<5'000'000, 80, 2048>();
	RunForAllThreads<5'000'000, 80, 1024>();
	RunForAllThreads<5'000'000, 80, 512>();
	RunForAllThreads<5'000'000, 80, 420>();
	RunForAllThreads<5'000'000, 80, 256>();
	RunForAllThreads<5'000'000, 80, 128>();
	RunForAllThreads<5'000'000, 80, 64>();
	RunForAllThreads<5'000'000, 80, 16>();

}