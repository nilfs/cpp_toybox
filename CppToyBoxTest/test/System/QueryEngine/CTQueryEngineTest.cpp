#include "stdafx.h"
#include <vector>
#include <iostream>
#include "System/QueryEngine/CTQueryEngine.h"

class CTQueryEngineTest : public ::testing::Test {
protected:
	CTQueryEngineTest() {
	}

	virtual ~CTQueryEngineTest() {
	}


	virtual void SetUp() {
	}

	virtual void TearDown() {
	}
};

TEST_F(CTQueryEngineTest, register_collection) {

//	CTSimpleQueryEngineFactory< std::vector<int> > factory;
	CTSimpleQueryTaskSystem< std::vector<int> > taskSystem;

	CTQueryEngine< std::vector<int> > engine(taskSystem);

	std::vector<int> data;
	data.resize(10);

	int n = 1;
	std::generate(data.begin(), data.end(), [&n]() { auto t = n; n += 1; return t; });
	engine.SetData( data );
	std::future< std::vector<int> > queryHandle = engine.RegisterQuery([]( const std::vector<int>& data ){ 
		
		std::vector<int> retval;

		for (auto v : data) {
			if (v % 2 == 0) {
				retval.push_back(v);
			}
			else {
				//none
			}
		}

		return retval;
	});

	taskSystem.Update();

	{
		std::cout << "------" << std::endl;

		auto result = queryHandle.get();
		for (auto v : result)
		{
			std::cout << v << std::endl;
		}
	}
 }