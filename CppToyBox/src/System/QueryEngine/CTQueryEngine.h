#pragma once

#include <vector>
#include <functional>
#include <future>
#include <memory>

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
class CTFunctorQueryIns : public CTQueryInsBase< ContainerType >
{
private:
	std::promise<ResultType> m_result;
	Func& m_func;

public:
	CTFunctorQueryIns(ContainerType& c, Func& f)
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

template< typename QueryInsBase >
class CTQueryTaskSystem
{
public:
	virtual void RegisterQuery(std::shared_ptr<QueryInsBase> query) = 0;
};

template< typename ContainerType >
class CTQueryEngine
{
public:
	using QueryInsBaseType = CTQueryInsBase< ContainerType >;
	using TaskSystem = CTQueryTaskSystem< QueryInsBaseType >;

private:
	std::vector< std::shared_ptr<QueryInsBaseType> > m_queries;

	TaskSystem& m_taskSystem;
	ContainerType m_data;

public:
	CTQueryEngine(TaskSystem& taskSystem)
		:m_taskSystem(taskSystem)
	{}
	~CTQueryEngine()
	{
		m_queries.clear();
	}

public:

	void SetData(const ContainerType& c) { m_data = c; }

	template< typename Func, typename ResultType = my_result_of< Func > >
	std::future< ResultType > RegisterQuery(Func f)
	{
		auto q = std::make_shared< CTFunctorQueryIns<ContainerType, Func, ResultType> >(m_data, f);

		m_taskSystem.RegisterQuery(q);

		m_queries.push_back(q);
		return q->GetFuture();
	}
};

template< typename ContainerType, typename Func, typename ResultType = my_result_of< Func > >
class CTSimpleQueryIns : public CTFunctorQueryIns< ContainerType, Func, ResultType>
{

};

// Simple Task System for background task.
// Query runs on main thread
template< typename ContainerType>
class CTBackgroundQueryTaskSystem : public CTQueryTaskSystem< CTQueryInsBase<ContainerType> >
{
private:
	std::thread m_queryExecThread;
	std::vector< std::shared_ptr< CTQueryInsBase<ContainerType> > > m_queries;

public:
	~CTBackgroundQueryTaskSystem()
	{
		if (m_queryExecThread.joinable())
		{
			// wait for prev task
			m_queryExecThread.join();
		}
	}

public:
	virtual void RegisterQuery(std::shared_ptr< CTQueryInsBase<ContainerType> > query)
	{
		m_queries.push_back(query);
	}

public:
	void Update()
	{
		if (m_queryExecThread.joinable())
		{
			// wait for prev task
			m_queryExecThread.join();
		}

		m_queryExecThread = std::thread([this]() {
			for (auto query : m_queries)
			{
				query->Exec();
			}
		});
	}
};

// Simple Task System.
// Query runs on main thread
template< typename ContainerType>
class CTSimpleQueryTaskSystem : public CTQueryTaskSystem< CTQueryInsBase<ContainerType> >
{
private:
	std::vector< std::shared_ptr< CTQueryInsBase<ContainerType> > > m_queries;

public:
	virtual void RegisterQuery(std::shared_ptr< CTQueryInsBase<ContainerType> > query)
	{
		m_queries.push_back(query);
	}

public:
	void Update()
	{
		for (auto query : m_queries)
		{
			query->Exec();
		}
	}
};
