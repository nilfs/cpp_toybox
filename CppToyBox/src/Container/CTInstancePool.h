#pragma once

#include <vector>
#include <algorithm>

template< typename Instance >
class CTInstancePool
{
private:
	struct Buffer {
	public:
		enum class Status : uint8_t {
			Unuse,
			Used,
		};

	public:
		Status m_status : 7;
		bool m_end : 1;//end of instance pool
		Instance m_value;

	public:
		Buffer()
			:m_status(false)
		{}
		Buffer(const Status status, const Instance& value)
			:m_status(status)
			,m_value(std::move(value))
		{}
	};

public:
	class Handle {
	public:
		static const uint16_t InvalidIndex = UINT16_MAX;

	private:
		uint16_t m_index;
		uint16_t m_recycleCount;

	public:
		Handle(const uint16_t index, const uint16_t recycleCount)
			:m_index(index)
			,m_recycleCount(recycleCount)
		{}

	public:
		uint16_t get_index() const { return m_index; }
		bool is_valid(const uint16_t recycleCount) const { return m_index != InvalidIndex && m_recycleCount == recycleCount; }
	};

public:
	class Iterator {
	private:
		typename std::vector< Buffer >::iterator m_ite;

	public:
		Iterator( typename std::vector< Buffer >::iterator ite )
			:m_ite(ite)
		{}

	public:
		Iterator& operator++() {
			++m_ite;
			return *this;
		}
		Iterator& operator--() {
			--m_ite;
			return *this;
		}

		Instance& operator*() { return m_ite->m_value; }
		Instance* operator->() { return &m_ite->m_value; }

		bool operator==(const Iterator rhs) const {
			return m_ite == rhs.m_ite;
		}
		bool operator!=(const Iterator rhs) const {
			return m_ite != rhs.m_ite;
		}
	};

private:
	uint32_t m_usedSize;
	uint32_t m_freeSize;
	uint16_t m_recycleCount;
	std::vector< Buffer > m_buffers;

public:
	CTInstancePool()
		:m_usedSize(0)
		,m_freeSize(0)
		,m_recycleCount(0)
		//,m_buffers()
	{

	}
	~CTInstancePool() {}

public:
	Iterator begin() { return Iterator(m_buffers.begin()); }
	Iterator end() { return Iterator(m_buffers.end()); }

	bool empty() const { return size() == 0; }

	size_t size() const { return m_usedSize; }

	void reserve(size_t s) { m_buffers.reserve(s); }

	Instance& operator[](const Handle& h) {
		return m_buffers[h.get_index()].m_value;
	}

	Handle add(const Instance& ins) {

		++m_recycleCount;

		uint16_t createdIndex = Handle::InvalidIndex;

		if (0 < m_freeSize) {
			auto retIte = std::find_if(m_buffers.begin(), m_buffers.end(), [](const auto& buffer) {
				return buffer.m_status == Buffer::Status::Unuse;
			});
			assert(retIte != m_buffers.end());
			new(&retIte->m_value) Instance(ins);
			retIte->m_status = Buffer::Status::Used;
			--m_freeSize;

			createdIndex = static_cast<int16_t>(retIte - m_buffers.begin());
		}
		else {
			// unmark end of buffer
			if( 0 < m_buffers.size() ) {
				(--m_buffers.end())->m_end = false;
			}

			m_buffers.emplace_back(Buffer::Status::Used, ins );
			createdIndex = static_cast<int16_t>(m_buffers.size() - 1);

			// mark end of buffer
			(--m_buffers.end())->m_end = true;
		}

		++m_usedSize;

		return Handle(createdIndex, m_recycleCount);
	}

	void remove(const Handle& handle ) {
	}

public:// util methods
	Iterator to_iterator(const Handle& h) {
		return Iterator(m_buffers.begin() + h.get_index());
	}

private:

};

