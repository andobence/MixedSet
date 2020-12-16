#pragma once
#include <atomic>
#include <bit>
#include <unordered_set>
#include <mutex>
#include "List.h"

inline uint32_t reverse(uint32_t x)
{
	x = ((x >>  1) & 0x5555'5555ul) | ((x & 0x5555'5555ul) <<  1);
	x = ((x >>  2) & 0x3333'3333ul) | ((x & 0x3333'3333ul) <<  2);
	x = ((x >>  4) & 0x0f0f'0f0ful) | ((x & 0x0f0f'0f0ful) <<  4);
	x = ((x >>  8) & 0x00ff'00fful) | ((x & 0x00ff'00fful) <<  8);
	x = ((x >> 16) & 0x0000'fffful) | ((x & 0x0000'fffful) << 16);
	return x;
}

inline uint64_t reverse(uint64_t x)
{
	x = ((x >> 1) & 0x5555'5555'5555'5555ull) | ((x & 0x5555'5555'5555'5555ull) << 1);
	x = ((x >> 2) & 0x3333'3333'3333'3333ull) | ((x & 0x3333'3333'3333'3333ull) << 2);
	x = ((x >> 4) & 0x0f0f'0f0f'0f0f'0f0full) | ((x & 0x0f0f'0f0f'0f0f'0f0full) << 4);
	x = ((x >> 8) & 0x00ff'00ff'00ff'00ffull) | ((x & 0x00ff'00ff'00ff'00ffull) << 8);
	x = ((x >> 16) & 0x0000'ffff'0000'ffffull) | ((x & 0x0000'ffff'0000'ffffull) << 16);
	x = ((x >> 32) & 0x0000'0000'ffff'ffffull) | ((x & 0x0000'0000'ffff'ffffull) << 32);
	return x;
}

template<typename T, std::size_t BlockSize = 128, class Hasher = std::hash<T>>
class HashSet
{
	using Mutex = std::shared_mutex;
	using UniqueLock = std::unique_lock<Mutex>;
	using SharedLock = std::shared_lock<Mutex>;
	using Hash = uint64_t;
	using Bucket = List<std::pair<Hash, T>, BlockSize>;

public:
	HashSet(size_t startBucketSize = 32, Hasher hasher = {})
		: m_hasher(std::move(hasher))
	{
		assert(startBucketSize > 0);
		for (size_t i = 0; i < startBucketSize; i++)
		{
			m_buckets.emplace_back(std::make_unique<Bucket>());
		}
	}

	bool insert(T elem)
	{
		auto hash = m_hasher(elem);
		auto reverseHash = reverse(hash);

		SharedLock lock{ m_bucketsMutex };

		auto bucketIdx = bucket(hash);

		bool ret = m_buckets[bucketIdx]->insert({ reverseHash, elem });
		if (ret) m_size++;

		if (private_load_factor() > max_load_factor())
		{
			lock.unlock();
			try_extend_buckets();
		}

		return ret;
	}

	bool erase(const T& elem)
	{
		auto hash = m_hasher(elem);
		auto reverseHash = reverse(hash);

		SharedLock lock{ m_bucketsMutex };

		auto bucketIdx = bucket(hash);
		
		bool ret = m_buckets[bucketIdx]->erase({ reverseHash, elem });
		if (ret) m_size--;

		return ret;
	}

	bool contains(const T& elem)
	{
		auto hash = m_hasher(elem);
		auto reverseHash = reverse(hash);

		SharedLock lock{ m_bucketsMutex };

		auto bucketIdx = bucket(hash);

		return m_buckets[bucketIdx]->contains({ reverseHash, elem });
	}

	float load_factor() const
	{
		SharedLock lock{ m_bucketsMutex };

		return private_load_factor();
	}

	float max_load_factor() const
	{
		return m_maxLoadFactor;
	}

	void max_load_factor(float ml)
	{
		m_maxLoadFactor = ml;
	}

private:
	// Assumption: a shared lock for m_bucketsMutex is acquired.
	size_t bucket(Hash hash) const
	{
		// Examples:
		// Size -> Mask
		// 0 -> bit_ceil(00000000)-1 = 00000000
		// 1 -> bit_ceil(00000001)-1 = 00000000
		// 2 -> bit_ceil(00000010)-1 = 00000001
		// 3 -> bit_ceil(00000011)-1 = 00000011
		// 4 -> bit_ceil(00000100)-1 = 00000011
		// 5 -> bit_ceil(00000101)-1 = 00000111
		// 6 -> bit_ceil(00000110)-1 = 00000111
		// 7 -> bit_ceil(00000111)-1 = 00000111
		// 8 -> bit_ceil(00001000)-1 = 00000111
		// 9 -> bit_ceil(00001001)-1 = 00001111
		auto mask = std::bit_ceil(m_buckets.size()) - 1;
		auto bucketIdx = hash & mask;

		if (bucketIdx >= m_buckets.size())
			bucketIdx = hash & (mask >> 1);

		assert(bucketIdx < m_buckets.size());

		return bucketIdx;
	}

	// Assumption: a shared lock for m_bucketsMutex is acquired.
	float private_load_factor() const
	{
		return m_size / static_cast<float>(m_buckets.size());
	}

	void try_extend_buckets()
	{
		UniqueLock lock{ m_bucketsMutex };
		
		if (private_load_factor() <= max_load_factor())
			return;

		size_t newBucketIdx = m_buckets.size();
		size_t lowerBucket = newBucketIdx - std::bit_floor(newBucketIdx);
		auto& newBucket = m_buckets.emplace_back(std::make_unique<Bucket>());

		Hash hash = reverse(static_cast<Hash>(newBucketIdx));
		m_buckets[lowerBucket]->split_after(*newBucket, [&hash](const std::pair<Hash, T>& p)
		{
			return p.first >= hash;
		});
	}

	std::atomic<std::size_t> m_size;
	std::vector<std::unique_ptr<Bucket>> m_buckets;
	mutable Mutex m_bucketsMutex;
	Hasher m_hasher;

	std::atomic<float> m_maxLoadFactor = 512.;
};
