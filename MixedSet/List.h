#pragma once

#include <cassert>
#include <memory>
#include <shared_mutex>
#include <array>
#include <algorithm>

template<
	typename T,
	std::size_t Size = 128,
	typename Less = std::less<T>
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
		SharedLock currentLock{ m_headMutex }, nextLock;
		auto currentNode = m_head;
		if (currentNode) nextLock = SharedLock{ currentNode->m_mutex };
		while (currentNode)
		{
			currentLock = std::move(nextLock);
			auto nextNode = currentNode->m_next;
			if (nextNode)
				nextLock = SharedLock{ nextNode->m_mutex };

			auto valueIter = currentNode->find(value);
			if (valueIter != currentNode->end() && *valueIter == value)
				return true;

			currentNode = nextNode;
		}
		return false;
	}

	std::size_t size() const
	{
		return m_size.load();
	}

	template<typename F>
	void split_after(List& upperPart, F&& f)
	{
		UniqueLock currentLock{ m_headMutex };
		auto currentNode = m_head;
		UniqueLock nextLock{ currentNode->m_mutex };
		decltype(currentNode) tail;
		while (currentNode)
		{
			tail = currentNode;
			currentLock = std::move(nextLock);
			auto nextNode = currentNode->m_next;
			if (nextNode) nextLock = UniqueLock{ nextNode->m_mutex };

			auto iter = currentNode->partition_point(std::forward<F>(f));
			auto currentEnd = currentNode->end();
			if (iter != currentNode->end())
			{
				auto newNode = std::make_shared<Node>();
				std::copy(iter, currentEnd, newNode->begin());
				newNode->m_size = currentEnd - iter;
				newNode->m_next = nextNode;
				currentNode->m_size = iter - currentNode->begin();
				currentNode->m_next = nullptr;
				upperPart.m_head = std::move(newNode);
				upperPart.recalculateSize();
				m_size -= upperPart.m_size;
				return;
			}

			currentNode = nextNode;
		}

		// If the partition point wasn't found until this point,
		// return an empty list as the second partition
		upperPart.m_head = std::make_shared<Node>();
		upperPart.m_size = 0;
	}

private:
	void recalculateSize()
	{
		std::size_t size{ 0 };
		SharedLock currentLock;
		auto currentNode = m_head;
		while (currentNode)
		{
			size += currentNode->size();
			auto nextNode = currentNode->m_next;
			currentLock = SharedLock{ nextNode->m_mutex };
			currentNode = nextNode;
		}
		m_size = size;
	}

	bool insert_private(const T& value)
	{
		UniqueLock currentLock{ m_headMutex };
		auto currentNode = m_head;
		UniqueLock nextLock{ currentNode->m_mutex };
		decltype(currentNode) tail;
		while (currentNode)
		{
			tail = currentNode;
			currentLock = std::move(nextLock);
			auto nextNode = currentNode->m_next;
			if (nextNode) nextLock = UniqueLock{ nextNode->m_mutex };

			auto valueIter = currentNode->find(value);
			if (valueIter != currentNode->end())
			{
				if (*valueIter == value)
					return false;

				if (currentNode->m_size < Size)
				{
					currentNode->insert(value, valueIter);
				}
				else
				{
					currentNode->insert_and_split(value, valueIter);
				}

				return true;
			}
			else if (valueIter != currentNode->m_contents.end() &&
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
		UniqueLock prevLock, currentLock{ m_headMutex };
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
					return false;

				currentNode->erase(valueIter);
				if (currentNode->m_size == 0)
				{
					if (prevNode)
					{
						prevNode->m_next = currentNode->m_next;
					}
					else if (m_head->m_next) // !prevNode ==> currentNode == m_head
					{
						m_head = m_head->m_next; // remove the first list element
					}
				}
				currentLock.unlock();
				return true;
			}
			else 
            {
                if (auto nextNode = currentNode->m_next)
                {
                    auto nextLock = SharedLock{ nextNode->m_mutex };
                    if (nextNode->m_contents.front() > value)
                    {
                        return false;
                    }
                }
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

		auto size() -> std::size_t
		{
			return end() - begin();
		}

		auto find(const T& value)
		{
			return std::lower_bound(begin(), end(), value, Less());
		}

		template<typename Predicate>
		auto partition_point(Predicate&& p)
		{
			return std::partition_point(begin(), end(), std::forward<Predicate>(p));
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

		void insert_and_split(const T& value, typename decltype(m_contents)::iterator iter)
		{
			auto newNode = std::make_shared<Node>();
			newNode->m_next = m_next;
			std::size_t pos = iter - m_contents.begin();
			std::size_t midPos = m_size / 2;

			if (pos < midPos)
			{
				std::copy(
					m_contents.begin() + midPos,
					m_contents.end(),
					newNode->m_contents.begin());
				newNode->m_size = m_size - midPos;
				m_size = midPos;
				insert(value, iter);
			}
			else
			{
				std::copy(
					m_contents.begin() + midPos,
					m_contents.begin() + pos,
					newNode->m_contents.begin());
				newNode->m_contents[pos - midPos] = value;
				std::copy(
					m_contents.begin() + pos,
					m_contents.end(),
					newNode->m_contents.begin() + pos - midPos + 1);
				m_size = midPos;
				newNode->m_size = Size - midPos + 1;
			}

			m_next = newNode;
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

	mutable Mutex m_headMutex;
	std::shared_ptr<Node> m_head;
	std::atomic<std::size_t> m_size;
};