#pragma once
#include <atomic>
#include <vector>

class BitVectorSet
{
public:
	BitVectorSet(size_t size) : m_data((size + 7) / 8), m_size(size)
	{
	}

	bool insert(size_t index)
	{
		if (index >= m_size)
			return false;

		auto& byte = m_data[index / 8];
		uint8_t bitMask = 1 << (index % 8);

		uint8_t oldValue = byte.load(std::memory_order_relaxed);
		uint8_t newValue = 0;
		do
		{
			if (oldValue & bitMask)
				return false;

			newValue = oldValue | bitMask;
		} while (!byte.compare_exchange_strong(oldValue, newValue,
			std::memory_order_release,
			std::memory_order_relaxed));

		return true;
	}

	bool erase(size_t index)
	{
		if (index >= m_size)
			return false;

		auto& byte = m_data[index / 8];
		uint8_t bitMask = 1 << (index % 8);

		uint8_t oldValue = byte.load(std::memory_order_relaxed);
		uint8_t newValue = 0;
		do
		{
			if (!(oldValue & bitMask))
				return false;

			newValue = oldValue & ~bitMask;
		} while (!byte.compare_exchange_strong(oldValue, newValue,
			std::memory_order_release,
			std::memory_order_relaxed));

		return true;
	}

	bool contains(size_t index)
	{
		if (index >= m_size)
			return false;

		auto& byte = m_data[index / 8];
		uint8_t bitMask = 1 << (index % 8);

		uint8_t oldValue = byte.load(std::memory_order_relaxed);

		return oldValue & bitMask;
	}

private:
	std::vector<std::atomic<uint8_t>> m_data;
	size_t m_size;
};