#pragma once

#include <vector>
#include <algorithm>

template< typename Value >
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
		Value m_value;

	public:
		Buffer()
			:m_status(false)
		{}
		Buffer(const Status status, const Value& value)
			:m_status(status)
			,m_value(std::move(value))
		{}
	};

public:
	class handle {
	public:
		static const uint16_t InvalidIndex = UINT16_MAX;

	private:
		uint16_t m_index;
		uint16_t m_recycleCount;

	public:
		handle(const uint16_t index, const uint16_t recycleCount)
			:m_index(index)
			,m_recycleCount(recycleCount)
		{}

	public:
		uint16_t get_index() const { return m_index; }
		bool is_valid(const uint16_t recycleCount) const { return m_index != InvalidIndex && m_recycleCount == recycleCount; }
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
	bool empty() const { return size() == 0; }

	size_t size() const { return m_usedSize; }

	void reserve(size_t s) { m_buffers.reserve(s); }

	Value& operator[](const handle& h) {
		return m_buffers[h.get_index()].m_value;
	}

	handle add(const Value& v) {

		++m_recycleCount;

		uint16_t createdIndex = handle::InvalidIndex;

		if (0 < m_freeSize) {
			auto retIte = std::find_if(m_buffers.begin(), m_buffers.end(), [](const auto& buffer) {
				return buffer.m_status == Buffer::Status::Unuse;
			});
			assert(retIte != m_buffers.end());
			new(&retIte->m_value) Value(v);
			retIte->m_status = Buffer::Status::Used;
			--m_freeSize;

			createdIndex = static_cast<int16_t>(retIte - m_buffers.begin());
		}
		else {
			m_buffers.emplace_back(Buffer::Status::Used, v );
			createdIndex = static_cast<int16_t>(m_buffers.size() - 1);
		}

		++m_usedSize;

		return handle(createdIndex, m_recycleCount);
	}
};

