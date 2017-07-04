#pragma once

#include <vector>
#include <algorithm>

#define CTInstancePool_Assert_ValidHandle(handle) assert(_check_valid_handle(handle))


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
		using InstanceType = Instance;

	public:
		Status m_status : 6;
		bool m_begin : 1;//begin position of instance pool
		bool m_end : 1;//end position of instance pool
		uint16_t m_recycleCount;//value for determining whether it is regenerated buffer
		alignas( alignof(Instance) ) char m_value[sizeof(Instance)];

	public:
		Buffer()
			:m_status(Status::Unuse)
			,m_begin(false)
			,m_end(false)
			,m_recycleCount(0)
			//,m_value()
		{}
		template<class... Valty>
		Buffer(const Status status, uint16_t recycleCount, Valty&&... val)
			:m_status(status)
			,m_begin(false)
			,m_end(false)
			,m_recycleCount(recycleCount)
			//,m_value()
		{
			CT_PLACEMENT_NEW(m_value, Instance)((val)...);
		}
		Buffer(const Status status, uint16_t recycleCount, const Instance& instance)
			:m_status(status)
			,m_begin(false)
			,m_end(false)
			,m_recycleCount(recycleCount)
			//,m_value()
		{
			CT_PLACEMENT_NEW(m_value, Instance)(instance);
		}

	public:
		Instance& get_instance() { return *reinterpret_cast<Instance*>(m_value); }
		const Instance& get_instance() const { return *reinterpret_cast<const Instance*>(m_value); }
	};

public:
	class Handle {
	public:
		static const uint16_t InvalidIndex = UINT16_MAX;

	private:
		uint16_t m_index;//index of instance pool
		uint16_t m_recycleCount;//value for determining whether it is regenerated buffer

	public:
		Handle(const uint16_t index, const uint16_t recycleCount)
			:m_index(index)
			,m_recycleCount(recycleCount)
		{}

	public:
		uint16_t get_index() const { return m_index; }
		uint16_t get_recycle_count() const { return m_recycleCount; }
		bool is_valid(const uint16_t recycleCount) const { return m_index != InvalidIndex && m_recycleCount == recycleCount; }
	};

private:
	template< typename BufferIteratorType>
	class _BaseIterator {
		friend CTInstancePool;//for to_handle() to implement

	public:
		using iterator_category = typename std::random_access_iterator_tag;
		using value_type = typename std::iterator_traits<BufferIteratorType>::value_type::InstanceType;
		using difference_type = typename std::iterator_traits<BufferIteratorType>::difference_type;
		using pointer = typename const value_type*;
		using reference = typename const value_type&;

	private:
		BufferIteratorType m_ite;

	public:
		_BaseIterator(BufferIteratorType ite)
			:m_ite(ite)
		{}

	public:
		reference operator*() { return m_ite->get_instance(); }
		pointer operator->() { return &m_ite->get_instance(); }

		bool operator==(const _BaseIterator rhs) const {
			return m_ite == rhs.m_ite;
		}
		bool operator!=(const _BaseIterator rhs) const {
			return m_ite != rhs.m_ite;
		}

	public:

		_BaseIterator& operator++() {
			if (m_ite->m_end) {
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

		_BaseIterator operator+(uint32_t offset) {
			_BaseIterator retval(*this);

			for (uint32_t i = 0; i < offset; ++i) {
				++retval;
			}

			return retval;
		}
	};

	template< typename BufferIteratorType>
	class _ReverseBaseIterator {
	private:
		BufferIteratorType m_ite;

	public:
		_ReverseBaseIterator(BufferIteratorType ite)
			:m_ite(ite)
		{}

	public:
		auto& operator*() { return m_ite->get_instance(); }
		auto* operator->() { return &m_ite->get_instance(); }

		bool operator==(const _ReverseBaseIterator rhs) const {
			return m_ite == rhs.m_ite;
		}
		bool operator!=(const _ReverseBaseIterator rhs) const {
			return m_ite != rhs.m_ite;
		}

	public:
		_ReverseBaseIterator& operator++() {
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
	};


public:
	using Iterator = _BaseIterator< typename std::vector< Buffer >::iterator >;
	using ConstIterator = _BaseIterator< typename std::vector< Buffer >::const_iterator >;
	using ReverseIterator = _BaseIterator< typename std::vector< Buffer >::reverse_iterator >;
	using ConstReverseIterator = _BaseIterator< typename std::vector< Buffer >::const_reverse_iterator >;

private:
	uint32_t m_usedSize;
	uint32_t m_freeSize;
	uint16_t m_recycleCount;
	std::vector< Buffer > m_buffers;
	uint16_t m_bufferBeginIndex;
	uint16_t m_bufferEndIndex;

public:
	CTInstancePool()
		:m_usedSize(0)
		,m_freeSize(0)
		,m_recycleCount(0)
		//,m_buffers()
		,m_bufferBeginIndex(0)
		,m_bufferEndIndex(0)
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
	Iterator begin() { return Iterator(m_buffers.begin() + m_bufferBeginIndex); }
	ConstIterator begin() const { return ConstIterator(m_buffers.begin() + m_bufferBeginIndex); }
	ConstIterator cbegin() const { return ConstIterator(m_buffers.begin() + m_bufferBeginIndex); }

	Iterator end() { return to_iterator({ m_bufferEndIndex, 0 }); }
	ConstIterator end() const { return to_const_iterator({ m_bufferEndIndex, 0 }); }
	ConstIterator cend() const { return to_const_iterator({ m_bufferEndIndex, 0 }); }

	ReverseIterator rbegin() { return to_reverse_iterator({ m_bufferEndIndex - 1u, 0 }); }
	ConstReverseIterator rbegin() const { return to_const_reverse_iterator({ m_bufferEndIndex - 1u, 0 }); }
	ConstReverseIterator crbegin() const { return to_const_reverse_iterator({ m_bufferEndIndex - 1u, 0 }); }

	ReverseIterator rend() { return to_reverse_iterator({ m_bufferBeginIndex, 0 }); }
	ConstReverseIterator rend() const { return to_const_reverse_iterator({ m_bufferBeginIndex, 0 }); }
	ConstReverseIterator crend() const { return to_const_reverse_iterator({ m_bufferBeginIndex, 0 }); }

	size_t capacity() const { return m_buffers.capacity(); }

	bool empty() const { return size() == 0; }

	size_t size() const { return m_usedSize; }

	void reserve(size_t s) { m_buffers.reserve(s); }

	Instance& operator[](const Handle& h) {
		return m_buffers[h.get_index()].get_instance();
	}
	const Instance& operator[] (const Handle& h) const {
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
			retIte->m_recycleCount = m_recycleCount;
			--m_freeSize;

			createdIndex = static_cast<int16_t>(retIte - m_buffers.begin());

			if (createdIndex < m_bufferBeginIndex) {
				// new begin of buffer
				(m_buffers.begin() + m_bufferBeginIndex)->m_begin = false;
				retIte->m_begin = true;
				m_bufferBeginIndex = createdIndex;
			}
			if (m_bufferEndIndex < createdIndex) {
				// new end of buffer
			}
		}
		else {
			// unmark end of buffer
			if (0 < m_buffers.size()) {
				(--m_buffers.end())->m_end = false;
			}

			m_buffers.emplace_back(Buffer::Status::Used, m_recycleCount, (_Val)...);
			createdIndex = static_cast<int16_t>(m_buffers.size() - 1);

			// mark end of buffer
			m_buffers.begin()->m_begin = true;
			(--m_buffers.end())->m_end = true;
			m_bufferEndIndex = createdIndex + 1;
		}

		++m_usedSize;

		return Handle(createdIndex, m_recycleCount);
	}

	// remove element
	void remove(const Handle& handle) {
		CTInstancePool_Assert_ValidHandle(handle);

		auto& buffer = _get_buffer_unsafe(handle);
		if(buffer.m_begin) {
			_mark_next_begin_of_buffer(handle);
		}
		buffer.m_status = Buffer::Status::Unuse;

		if (buffer.m_end) {
			_mark_next_end_of_buffer(handle);
		}

		CT_PLACEMENT_DELETE(&buffer.get_instance());

		--m_usedSize;
		++m_freeSize;
	}

	// remove element
	void remove(const Iterator& ite) {
		remove(to_handle(ite));
	}
	// remove element
	void remove(const ConstIterator& ite) {
		remove(to_handle(ite));
	}
	// remove element
	void remove(const ReverseIterator& ite) {
		remove(to_handle(ite));
	}
	// remove element
	void remove(const ConstReverseIterator& ite) {
		remove(to_handle(ite));
	}

	// remove all element
	void clear()
	{
		for (Buffer& buffer : m_buffers) {
			if (buffer.m_status == Buffer::Status::Used) {
				buffer.m_status = Buffer::Status::Unuse;
				CT_PLACEMENT_DELETE(&buffer.get_instance());
			}
		}
		m_usedSize = 0;
		m_freeSize = static_cast<uint32_t>(m_buffers.size());
		m_bufferEndIndex = 0;
	}

public:// util methods
	Iterator to_iterator(const Handle& h) {
		return Iterator(m_buffers.begin() + h.get_index());
	}
	ConstIterator to_iterator(const Handle& h) const {
		return ConstIterator(m_buffers.begin() + h.get_index());
	}
	ConstIterator to_const_iterator(const Handle& h) const {
		return ConstIterator(m_buffers.begin() + h.get_index());
	}

	ReverseIterator to_reverse_iterator(const Handle& h) {
		return ReverseIterator(m_buffers.rbegin() + (m_buffers.size() - h.get_index() - 1));
	}
	ConstReverseIterator to_reverse_iterator(const Handle& h) const {
		return ConstReverseIterator(m_buffers.rbegin() + (m_buffers.size() - h.get_index() - 1));
	}
	ConstReverseIterator to_const_reverse_iterator(const Handle& h) const {
		return ConstReverseIterator(m_buffers.rbegin() + (m_buffers.size() - h.get_index() - 1));
	}

	Handle to_handle(const Iterator& ite) const {
		return Handle(static_cast<uint16_t>(ite.m_ite - begin().m_ite), ite.m_ite->m_recycleCount);
	}
	Handle to_handle(const ConstIterator& ite) const {
		return Handle(static_cast<uint16_t>(ite.m_ite - begin().m_ite), ite.m_ite->m_recycleCount);
	}
	Handle to_handle(const ReverseIterator& ite) const {
		auto a = ite.m_ite - rbegin().m_ite;
		return Handle(static_cast<uint16_t>(m_buffers.size() - a - 1), ite.m_ite->m_recycleCount);
	}
	Handle to_handle(const ConstReverseIterator& ite) const {
		return Handle(static_cast<uint16_t>(ite.m_ite - rbegin().m_ite - m_buffers.size() - 1), ite.m_ite->m_recycleCount);
	}

	bool is_valid( const Handle& handle ) const {
		return _check_valid_handle( handle );
	}

private:
	Buffer& _get_buffer_unsafe(const Handle& handle) { return m_buffers[handle.get_index()]; }

	// mark new end instance.
	// find begin of buffer from `ite`
	void _mark_next_begin_of_buffer(const Handle& handle)
	{
		auto ite = m_buffers.begin() + handle.get_index() + 1;
		while (ite != m_buffers.end()) {

			if (ite->m_status == Buffer::Status::Used) {
				ite->m_begin = true;
				m_bufferBeginIndex = std::distance(m_buffers.begin(), ite);
				break;
			}
			++ite;
		}
	}

	// mark new end instance.
	// find end of buffer from `ite`
	void _mark_next_end_of_buffer(const Handle& handle)
	{
		auto ite = m_buffers.rbegin() + (m_buffers.size() - handle.get_index());
		while (ite != m_buffers.rend()) {

			if (ite->m_status == Buffer::Status::Used) {
				ite->m_end = true;
				m_bufferEndIndex = static_cast<uint16_t>(m_buffers.size() - std::distance(m_buffers.rbegin(), ite));
				break;
			}
			++ite;
		}
	}

	bool _check_valid_handle(const Handle& handle) const {
		bool retval = true;
		if (m_buffers.size() < handle.get_index()) {
			// probably old or broken handle.
			retval = false;
		}
		else{
			const auto& buffer = m_buffers[handle.get_index()];
			if (buffer.m_status != Buffer::Status::Used) {
				// unused buffer
				retval = false;
			}
			else if (buffer.m_recycleCount != handle.get_recycle_count()) {
				// probably old or broken handle.
				retval = false;
			}
		}
		return retval;
	}
};

