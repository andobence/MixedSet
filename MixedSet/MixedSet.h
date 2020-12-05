#pragma once
#include "BitVectorSet.h"
#include "HashSet.h"

template<typename T, typename Linearizer>
class MixedSet
{
public:
	MixedSet(Linearizer linearizer = {})
		: m_linearizer(std::move(linearizer)), m_bitvector(Linearizer::size)
	{
	}

	bool insert(T elem)
	{
		auto index = m_linearizer(elem);

		if (index.has_value())
			return m_bitvector.insert(*index);
		else
			return m_set.insert(std::move(elem));
	}

	bool erase(const T& elem)
	{
		auto index = m_linearizer(elem);

		if (index.has_value())
			return m_bitvector.erase(*index);
		else
			return m_set.erase(std::move(elem));
	}

	bool contains(const T& elem)
	{
		auto index = m_linearizer(elem);

		if (index.has_value())
			return m_bitvector.contains(*index);
		else
			return m_set.contains(std::move(elem));
	}

private:
	Linearizer m_linearizer;
	BitVectorSet m_bitvector;
	HashSet<T> m_set;
};
