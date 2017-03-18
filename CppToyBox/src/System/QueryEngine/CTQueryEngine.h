#pragma once

#include <vector>
#include <functional>
#include <future>

//from http://stackoverflow.com/questions/6512019/can-we-get-the-type-of-a-lambda-argument/35348334#35348334
template<typename Ret, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(F::*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Arg first_argument_helper(Ret(F::*) (Arg, Rest...) const);

template <typename F>
decltype(first_argument_helper(&F::operator())) first_argument_helper(F);

template <typename T>
using first_argument = decltype(first_argument_helper(std::declval<T>()));



template<typename Ret, typename Arg, typename... Rest>
Ret result_of_helper(Ret(*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Ret result_of_helper(Ret(F::*) (Arg, Rest...));

template<typename Ret, typename F, typename Arg, typename... Rest>
Ret result_of_helper(Ret(F::*) (Arg, Rest...) const);

template <typename F>
decltype(result_of_helper(&F::operator())) result_of_helper(F);

template <typename T>
using my_result_of = decltype(result_of_helper(std::declval<T>()));


template< typename ContainerType >
class CTQueryInsBase
{
protected:
	ContainerType& m_container;

public:
	CTQueryInsBase(ContainerType& c)
		:m_container(c)
	{}

public:
	virtual void Exec() = 0;
};

template< typename ContainerType, typename Func, typename ResultType = my_result_of< Func > >
class CTQueryIns : public CTQueryInsBase< ContainerType >
{
private:
	std::promise<ResultType> m_result;
	Func& m_func;

public:
	CTQueryIns(ContainerType& c, Func& f)
	:CTQueryInsBase< ContainerType >(c)
		,m_func(f)
	{}

public:
	void Exec() override
	{
		m_result.set_value(m_func(m_container));
	}

public:
	std::future<ResultType> GetFuture() { return m_result.get_future(); }
};



template< typename ContainerType >
class CTQueryEngine
{
private:
	std::vector< CTQueryInsBase<ContainerType>* > m_queries;

	ContainerType m_data;

public:
	CTQueryEngine() {}
	~CTQueryEngine()
	{
		for (auto* query : m_queries)
		{
			delete query;
		}
		m_queries.clear();
	}

public:

	void SetData(const ContainerType& c) { m_data = c; }

	template< typename Func, typename ResultType = my_result_of< Func > >
	std::future< ResultType > RegisterQuery(Func f)
	{
		auto q = new CTQueryIns<ContainerType, Func, ResultType>(m_data, f);
		m_queries.push_back(q);
		return q->GetFuture();
	}

public:
	void Update()
	{
		for (auto* query : m_queries)
		{
			query->Exec();
		}
	}
};
