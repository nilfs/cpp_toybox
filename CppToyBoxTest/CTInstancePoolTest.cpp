#include "stdafx.h"
#include "Container/CTInstancePool.h"
#include "ReuseableVectorTest.h"

struct TestData {

public:
	static int32_t s_constructCounter;
	static int32_t s_destructCounter;

public:
	int32_t m_value;

public:
	TestData(int32_t v)
	:m_value(v)
	{
//		std::cout << "TestData construct" << std::endl;
		++s_constructCounter;
	}

	~TestData() {
//		std::cout << "TestData destruct" << std::endl;
		++s_destructCounter;
	}

public:
	static void TearDown() {
		s_constructCounter = 0;
		s_destructCounter = 0;
	}
};
int32_t TestData::s_constructCounter = 0;
int32_t TestData::s_destructCounter = 0;


class CTInstancePoolTest : public ::testing::Test {
protected:
	CTInstancePoolTest() {
	}

	virtual ~CTInstancePoolTest() {
	}


	virtual void SetUp() {
	}

	virtual void TearDown() {
		TestData::TearDown();
	}
};

TEST_F(CTInstancePoolTest, size) {

	CTInstancePool<TestData> v;

	v.add(TestData(100));
	v.add(TestData(200));
	v.add(TestData(300));

	ASSERT_EQ(v.size(), 3);
}

TEST_F(CTInstancePoolTest, empty) {

	CTInstancePool<TestData> v;

	ASSERT_EQ(v.empty(), true);

	v.add(TestData(100));

	ASSERT_EQ(v.empty(), false);
}

TEST_F(CTInstancePoolTest, handle_is_steady) {

	CTInstancePool<TestData> v;

	auto handle = v.add(TestData(100));
	ASSERT_EQ(handle.get_index(), 0);
	v.add(TestData(200));

	ASSERT_EQ(v[handle].m_value, 100);
}

TEST_F(CTInstancePoolTest, iterator) {

	CTInstancePool<TestData> v;

	auto handle = v.add(TestData(100));
	CTInstancePool<TestData>::Iterator it = v.to_iterator(handle);
	ASSERT_EQ(it->m_value, 100);
	ASSERT_EQ((*it).m_value, 100);

	ASSERT_EQ(v.begin(), it);
	ASSERT_EQ(++v.begin(), v.end());
}

TEST_F(CTInstancePoolTest, const_iterator) {

	CTInstancePool<TestData> v;
	auto handle = v.add(TestData(100));
	
	const auto& v2 = v;
	CTInstancePool<TestData>::ConstIterator it = v2.to_iterator(handle);

	ASSERT_EQ(it->m_value, 100);
	ASSERT_EQ((*it).m_value, 100);

	ASSERT_EQ(v2.begin(), it);
	ASSERT_EQ(++v2.begin(), v2.end());
	ASSERT_EQ(it, v.to_const_iterator(handle));
}

TEST_F(CTInstancePoolTest, reverse_iterator) {

	CTInstancePool<TestData> v;
	v.add(TestData(100));
	ASSERT_EQ(v.rbegin()->m_value, 100);

	v.add(TestData(200));
	auto handle = v.add(TestData(300));
	auto it = v.to_reverse_iterator(handle);
	ASSERT_EQ(it->m_value, 300);
	ASSERT_EQ((*it).m_value, 300);
}

TEST_F(CTInstancePoolTest, const_reverse_iterator) {

	CTInstancePool<TestData> v;
	v.add(TestData(100));
	v.add(TestData(200));
	auto handle = v.add(TestData(300));

	const auto& v2 = v;
	CTInstancePool<TestData>::ConstReverseIterator it = v2.to_reverse_iterator(handle);
	ASSERT_EQ(it->m_value, 300);
	ASSERT_EQ((*it).m_value, 300);

	ASSERT_EQ(it, v.to_const_reverse_iterator(handle));
}

TEST_F(CTInstancePoolTest, iterator_skip_unused_buffer) {

	{
		SCOPED_TRACE("erase 2nd value");

		CTInstancePool<TestData> v;

		v.add(TestData(100));
		auto removeTarget = v.add(TestData(200));
		v.add(TestData(300));

		v.remove(removeTarget);

		ASSERT_EQ(v.size(), 2);

		auto it = v.begin();
		ASSERT_EQ(it->m_value, 100);//first value
		++it;
		ASSERT_EQ(it->m_value, 300);//third value
		ASSERT_EQ(++it, v.end());
	}

	{
		SCOPED_TRACE("erase 3rd value");

		CTInstancePool<TestData> v;

		v.add(TestData(100));
		v.add(TestData(200));
		auto removeTarget = v.add(TestData(300));

		v.remove(removeTarget);

		ASSERT_EQ(v.size(), 2);

		auto it = v.begin();
		ASSERT_EQ(it->m_value, 100);//first value
		++it;
		ASSERT_EQ(it->m_value, 200);//second value
		ASSERT_EQ(++it, v.end());
	}
}

TEST_F(CTInstancePoolTest, reserve) {

	CTInstancePool<TestData> v;
	v.reserve(3);

	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(v.capacity(), 3);
	ASSERT_EQ(TestData::s_constructCounter, 0);
	ASSERT_EQ(TestData::s_destructCounter, 0);

	v.emplace_add(100);

	ASSERT_EQ(v.size(), 1);
	ASSERT_EQ(v.capacity(), 3);
	ASSERT_EQ(TestData::s_constructCounter, 1);
	ASSERT_EQ(TestData::s_destructCounter, 0);
}

TEST_F(CTInstancePoolTest, remove_instance) {
	{
		CTInstancePool<TestData> v;
		v.emplace_add(100);
		auto handle = v.emplace_add(200);
		v.emplace_add(300);
		v.emplace_add(400);
		v.emplace_add(500);
		ASSERT_EQ(v.size(), 5);

		// remove 200
		v.remove(handle);
		ASSERT_EQ(v.size(), 4);
/*
		// remove 100
		v.remove(v.begin());
		ASSERT_EQ(v.size(), 3);
		ASSERT_EQ(v.begin()->m_value, 300);

		// remove 300
		v.remove(v.cbegin());
		ASSERT_EQ(v.size(), 2);
		ASSERT_EQ(v.begin()->m_value, 400);
*/
	}
}

TEST_F(CTInstancePoolTest, repeat_addition_and_deletion) {

	CTInstancePool<TestData> v;
	v.emplace_add(100);
	v.emplace_add(200);

	v.remove(v.cbegin());
	ASSERT_EQ(v.begin()->m_value, 200);

	v.emplace_add(100);
	ASSERT_EQ(v.begin()->m_value, 100);

	// remove back
	v.remove(v.rbegin());
	ASSERT_EQ(v.rbegin()->m_value, 100);

	//// clear all use clear()
	//v.clear();
	//v.emplace_add(100);
	//ASSERT_EQ(v.begin()->m_value, 100);

	//// clear all use remove()
	//v.remove(v.begin());
	//v.emplace_add(100);
	//v.emplace_add(200);
	//ASSERT_EQ(v.begin()->m_value, 100);
}

TEST_F(CTInstancePoolTest, alignment) {

	struct TestData2 {
		int32_t a;
		int32_t b;

		TestData2(int32_t _a, int32_t _b)
			:a(_a)
			, b(_b)
		{

		}
	};
	const auto testDataAligment = alignof(TestData2);

	CTInstancePool<TestData2> v;
	auto handle = v.emplace_add(INT32_MAX, INT32_MAX);
	auto& data = v[handle];
	ASSERT_EQ((uintptr_t)(&data) % testDataAligment, 0);
}

TEST_F(CTInstancePoolTest, is_valid_handle) {

	CTInstancePool<TestData> v;

	auto handle1 = v.emplace_add(100);
	ASSERT_EQ(v.is_valid(handle1), true);
	v.remove(handle1);
	ASSERT_EQ(v.is_valid(handle1), false);

	auto handle2 = v.emplace_add(100);
	ASSERT_EQ(v.is_valid(handle1), false);
	ASSERT_EQ(v.is_valid(handle2), true);
}

TEST_F(CTInstancePoolTest, clear) {

	CTInstancePool<TestData> v;

	v.emplace_add(100);
	v.emplace_add(200);
	v.emplace_add(300);

	ASSERT_EQ(TestData::s_constructCounter, 3);
	ASSERT_EQ(TestData::s_destructCounter, 0);

	v.clear();

	ASSERT_EQ(TestData::s_destructCounter, 3);

	ASSERT_EQ(v.begin(), v.end());
}