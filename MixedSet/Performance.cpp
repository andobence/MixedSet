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
	int InnerPointsPercentage
>
double Benchmark()
{
	constexpr static int width = 2 * HalfWidth;
	constexpr static float p = InnerPointsPercentage / 100.f;
	MixedSet<vec3, Vec3Linearizer<HalfWidth>> set;
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
	size_t MaxThreads_
>
void RunTests()
{
	constexpr static size_t MaxThreads = MaxThreads_ + 1;
	auto helper = []<size_t... I>(std::index_sequence<I...> _)
	{
		std::array<double, MaxThreads> results;

		((I >= MinThreads && (
			results[I] = Benchmark<100000000, I, 600, 100>(),
			std::cout << I << "\n"
			)),
			...);

		for (size_t i = MinThreads; i < MaxThreads; ++i)
		{
			std::cout << "#" << i << "  :  " << results[i] << std::endl;
		}
	};

	helper(std::make_index_sequence<MaxThreads>());
}

void PerformanceTest()
{
	RunTests<1, 16>();
}