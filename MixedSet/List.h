#pragma once

#include <cassert>
#include <memory>
#include <shared_mutex>
#include <array>
#include <algorithm>

template<
	typename T,
	typename Less,
	std::size_t Size = 128
>
class List
{
	using Mutex = std::shared_mutex;
	using UniqueLock = std::unique_lock<Mutex>;
	using SharedLock = std::shared_lock<Mutex>;

public:
	List()
		:m_head{ std::make_shared<Node>() },
		m_size{ 0 }
	{

	}

	bool insert(const T& value)
	{
		bool ret = insert_private(value);
		if (ret) ++m_size;
		return ret;
	}

	bool erase(const T& value)
	{
		bool ret = erase_private(value);
		if (ret) --m_size;
		return ret;
	}

	bool contains(const T& value) const
	{
		SharedLock currentLock, nextLock;
		auto currentNode = m_head;
		if (currentNode) nextLock = SharedLock{ currentNode->m_mutex };
		while (currentNode)
		{
			currentLock = std::move(nextLock);
			auto nextNode = currentNode->m_next;
			if (nextNode) nextLock = SharedLock{ nextNode->m_mutex };
			auto valueIter = currentNode->find(value);
			if (valueIter != currentNode->m_contents.end() && *valueIter == value)
			{
				return true;
			}
			currentNode = nextNode;
		}
		return false;
	}

	std::size_t size() const
	{
		return m_size.load();
	}

private:

	bool insert_private(const T& value)
	{
		UniqueLock currentLock, nextLock;
		auto currentNode = m_head;
		if (currentNode) nextLock = UniqueLock{ currentNode->m_mutex };
		decltype(currentNode) tail;
		while (currentNode)
		{
			tail = currentNode;
			currentLock = std::move(nextLock);
			auto nextNode = currentNode->m_next;
			if (nextNode) nextLock = UniqueLock{ nextNode->m_mutex };

			auto& contents = currentNode->m_contents;
			auto valueIter = currentNode->find(value);
			if (valueIter != currentNode->end())
			{
				if (*valueIter == value)
				{
					return false;
				}
				else if (currentNode->m_size < Size)
				{
					currentNode->insert(value, valueIter);
					return true;
				}
				else
				{
					auto newNode = std::make_shared<Node>();
					newNode->m_next = nextNode;
					std::size_t pos = valueIter - contents.begin();
					std::size_t midPos = currentNode->m_size / 2;

					if (pos < midPos)
					{
						std::copy(
							contents.begin() + midPos,
							contents.end(),
							newNode->m_contents.begin());
						newNode->m_size = currentNode->m_size - midPos;
						currentNode->m_size = midPos;
						currentNode->insert(value, valueIter);
					}
					else
					{
						std::copy(
							contents.begin() + midPos,
							contents.begin() + pos,
							newNode->m_contents.begin());
						newNode->m_contents[pos - midPos] = value;
						std::copy(
							contents.begin() + pos,
							contents.end(),
							newNode->m_contents.begin() + pos - midPos + 1);
						currentNode->m_size = midPos;
						newNode->m_size = Size - midPos + 1;
					}

					currentNode->m_next = newNode;
					return true;
				}
			}
			else if (valueIter != contents.end() && 
				(!nextNode || value < nextNode->m_contents.front()))
			{
				*valueIter = value;
				++currentNode->m_size;
				return true;
			}

			currentNode = nextNode;
		}

		tail->m_next = std::make_shared<Node>(value);

		return true;
	}

	bool erase_private(const T& value)
	{
		UniqueLock prevLock, currentLock;
		auto currentNode = m_head;
		decltype(currentNode) prevNode;
		while (currentNode)
		{
			prevLock = std::move(currentLock);
			currentLock = UniqueLock{ currentNode->m_mutex };

			auto& contents = currentNode->m_contents;
			auto valueIter = currentNode->find(value);
			if (valueIter != currentNode->end())
			{
				if (*valueIter != value)
				{
					return false;
				}
				else
				{
					currentNode->erase(valueIter);
					if (currentNode->m_size == 0)
					{
						if (prevNode)
						{
							prevNode->m_next = currentNode->m_next;
						}
						else // !prevNode ==> currentNode == m_head
						{
							m_head = m_head->m_next; // remove the first list element
						}
					}
					currentLock.unlock();
					return true;
				}
			}
			else if (currentNode->m_next && currentNode->m_next->m_contents[0] > value)
			{
				return false;
			}

			prevNode = currentNode;
			currentNode = currentNode->m_next;
		}

		return false;
	}

	struct Node
	{
		std::size_t m_size{ 0 };
		std::array<T, Size> m_contents;
		std::shared_ptr<Node> m_next;
		Mutex m_mutex;

		Node() = default;
		Node(const T& initialValue)
			:m_size{ 1 }
		{
			m_contents[0] = initialValue;
		}

		auto begin()
		{
			return std::begin(m_contents);
		}

		auto end()
		{
			return begin() + m_size;
		}

		auto find(const T& value)
		{
			return std::lower_bound(begin(), end(), value, Less());
		}

		void insert(const T& value, typename decltype(m_contents)::iterator iter)
		{
			assert(m_size != Size);

			for (auto i = end(); i > iter; --i)
			{
				*i = *(i - 1);
			}
			*iter = value;
			++m_size;
		}

		void erase(typename decltype(m_contents)::iterator iter)
		{
			--m_size;
			auto n = end();
			for (auto i = iter; i < n; ++i)
			{
				*i = *(i + 1);
			}
		}
	};

	std::shared_ptr<Node> m_head;
	std::atomic<std::size_t> m_size;
};