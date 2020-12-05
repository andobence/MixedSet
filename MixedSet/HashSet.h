#pragma once
#include <atomic>
#include <unordered_set>
#include <mutex>

template<typename T>
class HashSet
{
public:
	HashSet() = default;

	bool insert(T elem)
	{
		std::unique_lock _(m_mutex);

		auto [it, inserted] = m_set.insert(std::move(elem));

		return inserted;
	}

	bool erase(const T& elem)
	{
		std::unique_lock _(m_mutex);

		return m_set.erase(elem) > 0;
	}

	bool contains(const T& elem)
	{
		std::unique_lock _(m_mutex);

		return m_set.count(elem) > 0;
	}

private:
	std::unordered_set<T> m_set;
	std::mutex m_mutex;
};
