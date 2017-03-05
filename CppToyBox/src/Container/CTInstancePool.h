#pragma once

#include <vector>
#include <algorithm>

#define CTInstancePool_Assert_ValidHandle(handle) assert(_check_valid_handle(handle))


template< typename Instance >
class CTInstancePool
{
public:
	struct Buffer {
	public:
		enum class Status : uint8_t {
			Unuse,
			Used,
		};

	public:
		Status m_status : 6;
		bool m_begin : 1;//begin position of instance pool
		bool m_end : 1;//end position of instance pool
		alignas( alignof(Instance) ) char m_value[sizeof(Instance)];

	public:
		Buffer()
			:m_status(Status::Unuse)
			,m_begin(false)
			,m_end(false)
			//,m_value()
		{}
		Buffer(Status status)
			:m_status(status)
			,m_begin(false)
			,m_end(false)
			//,m_value()
		{}
		template<class... Valty>
		Buffer(const Status status, Valty&&... val)
			:m_status(status)
			,m_begin(false)
			,m_end(false)
			//,m_value()
		{
			CT_PLACEMENT_NEW(m_value, Instance)((val)...);
		}
		Buffer(const Status status, const Instance& instance)
			:m_status(status)
			,m_begin(false)
			,m_end(false)
			//,m_value()
		{
			CT_PLACEMENT_NEW(m_value, Instance)(instance);
		}

	public:
		Instance& get_instance() { return *reinterpret_cast<Instance*>(m_value); }
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
			if( m_ite->m_end ){
				// end position
				++m_ite;
			}
			else {
				++m_ite;
				// skip unused buffer
				while (!m_ite->m_end && m_ite->m_status != Buffer::Status::Used) {
					++m_ite;
				}
			}
			return *this;
		}

		Instance& operator*() { return m_ite->get_instance(); }
		Instance* operator->() { return &m_ite->get_instance(); }

		bool operator==(const Iterator rhs) const {
			return m_ite == rhs.m_ite;
		}
		bool operator!=(const Iterator rhs) const {
			return m_ite != rhs.m_ite;
		}
	};
	class ReverseIterator {
	private:
		typename std::vector< Buffer >::reverse_iterator m_ite;

	public:
		ReverseIterator(typename std::vector< Buffer >::reverse_iterator ite)
			:m_ite(ite)
		{}

	public:
		ReverseIterator& operator++() {
			if (m_ite->m_begin) {
				// begin position
				++m_ite;
			}
			else {
				++m_ite;
				// skip unused buffer
				while (!m_ite->m_begin && m_ite->m_status != Buffer::Status::Used) {
					++m_ite;
				}
			}
			return *this;
		}

		Instance& operator*() { return m_ite->get_instance(); }
		Instance* operator->() { return &m_ite->get_instance(); }

		bool operator==(const ReverseIterator rhs) const {
			return m_ite == rhs.m_ite;
		}
		bool operator!=(const ReverseIterator rhs) const {
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
	~CTInstancePool() {
		for (Buffer& buffer : m_buffers) {
			if (buffer.m_status == Buffer::Status::Used) {
				CT_PLACEMENT_DELETE(&buffer.get_instance());
			}
		}
	}

public:// STL like methods
	Iterator begin() { return Iterator(m_buffers.begin()); }
	Iterator end() { return Iterator(m_buffers.end()); }
	ReverseIterator rbegin() { return ReverseIterator(m_buffers.rbegin()); }
	ReverseIterator rend() { return ReverseIterator(m_buffers.rend()); }

	size_t capacity() const { return m_buffers.capacity(); }

	bool empty() const { return size() == 0; }

	size_t size() const { return m_usedSize; }

	void reserve(size_t s) { m_buffers.reserve(s); }

	Instance& operator[](const Handle& h) {
		return m_buffers[h.get_index()].get_instance();
	}

	// add into element
	Handle add(const Instance& ins) {
		return emplace_add(ins);
	}

	// add by moving into element
	template<class... _Valty>
	Handle emplace_add(_Valty&&... _Val)
	{
		++m_recycleCount;

		uint16_t createdIndex = Handle::InvalidIndex;

		if (0 < m_freeSize) {

			auto retIte = std::find_if(m_buffers.begin(), m_buffers.end(), [](const auto& buffer) {
				return buffer.m_status == Buffer::Status::Unuse;
			});
			assert(retIte != m_buffers.end());
			CT_PLACEMENT_NEW(&retIte->m_value, Instance)((_Val)...);
			retIte->m_status = Buffer::Status::Used;
			--m_freeSize;

			createdIndex = static_cast<int16_t>(retIte - m_buffers.begin());
		}
		else {
			// unmark end of buffer
			if (0 < m_buffers.size()) {
				(--m_buffers.end())->m_end = false;
			}

			m_buffers.emplace_back(Buffer::Status::Used, (_Val)...);
			createdIndex = static_cast<int16_t>(m_buffers.size() - 1);

			// mark end of buffer
			m_buffers.begin()->m_begin = true;
			(--m_buffers.end())->m_end = true;
		}

		++m_usedSize;

		return Handle(createdIndex, m_recycleCount);
	}

	// remove element
	void remove(const Handle& handle) {
		CTInstancePool_Assert_ValidHandle(handle);

		auto& buffer = _get_buffer_unsafe(handle);
		buffer.m_status = Buffer::Status::Unuse;
		CT_PLACEMENT_DELETE(&buffer.get_instance());

		--m_usedSize;
		++m_freeSize;
	}

public:// util methods
	Iterator to_iterator(const Handle& h) {
		return Iterator(m_buffers.begin() + h.get_index());
	}

	ReverseIterator to_reverse_iterator(const Handle& h) {
		return ReverseIterator(m_buffers.rbegin() + h.get_index());
	}

	bool is_valid( const Handle& handle ) const {
		return _check_valid_handle( handle );
	}

private:
	Buffer& _get_buffer_unsafe(const Handle& handle) { return m_buffers[handle.get_index()]; }

	bool _check_valid_handle(const Handle& handle) const {
		bool retval = true;
		if (m_buffers.size() < handle.get_index()) {
			retval = false;
		}
		else if (m_buffers[handle.get_index()].m_status != Buffer::Status::Used) {
			retval = false;
		}
		return retval;
	}
};

