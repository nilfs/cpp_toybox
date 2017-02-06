#include "stdafx.h"
#include "Container/CTInstancePool.h"
#include "ReuseableVectorTest.h"

struct TestData {

public:
	int32_t m_value;

public:
	TestData(int32_t v)
	:m_value(v)
	{
	//	std::cout << "TestData construct" << std::endl;
	}

	~TestData() {
	//	std::cout << "TestData destruct" << std::endl;
	}
};

class CTInstancePoolTest : public ::testing::Test {
protected:
	CTInstancePoolTest() {
	}

	virtual ~CTInstancePoolTest() {
	}


	virtual void SetUp() {
	}

	virtual void TearDown() {
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