#pragma once

#include <random>
#include <iostream>
#include <chrono>
#include <utility>

inline int RandInt(int from, int to)
{
	//thread_local std::random_device rd;
	thread_local std::mt19937 gen(0); // constant seed for testing
	std::uniform_int_distribution<> distrib(from, to);
	return distrib(gen);
}

inline bool CoinFlip()
{
	return RandInt(0, 1);
}

inline int RandSign()
{
	return CoinFlip() ? 1 : -1;
}

// Returns true (p*100) % of the times
inline bool Percent(float p)
{
	thread_local std::random_device rd;
	thread_local std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distrib(0, 1);
	return distrib(gen) < p;
}