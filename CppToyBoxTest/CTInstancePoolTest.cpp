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

TEST_F(CTInstancePoolTest, reverse_iterator) {

	CTInstancePool<TestData> v;

	auto handle = v.add(TestData(100));
	auto it = v.to_reverse_iterator(handle);
	ASSERT_EQ(it->m_value, 100);
	ASSERT_EQ((*it).m_value, 100);

	ASSERT_EQ(v.rbegin(), it);
	ASSERT_EQ(++v.rbegin(), v.rend());
}

TEST_F(CTInstancePoolTest, iterator_skip_unused_buffer) {

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
		ASSERT_EQ(v.size(), 3);

		v.remove(handle);
		ASSERT_EQ(TestData::s_destructCounter, 1);
		ASSERT_EQ(v.size(), 2);
	}

	ASSERT_EQ(TestData::s_constructCounter, 3);
	ASSERT_EQ(TestData::s_destructCounter, 3);
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
	ASSERT_EQ(alignof(CTInstancePool<TestData2>::Buffer), testDataAligment);
	ASSERT_EQ(sizeof(CTInstancePool<TestData2>::Buffer), 12);

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