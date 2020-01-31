#pragma once

#include <functional>

//WARNING : nothing is test in this file 
template <typename ReturnType, typename ...Parameters>
struct Function {

protected:
	typedef ReturnType(*StaticDelegate)(Parameters...);

	StaticDelegate delegate;

public:

	virtual void operator=(void* functionPointer)
	{
		delegate = static_cast<StaticDelegate>(functionPointer);
	}

	virtual ReturnType operator()(Parameters ...params)
	{
		return (*delegate)(std::forward<Parameters>(params)...);
	}
};

template <typename ClassType,typename ReturnType, typename ...Parameters>
struct ClassFunction {
	typedef ReturnType(ClassType::* ClassDelegate)(Parameters...);

	ClassDelegate delegate;

	void operator=(void* functionPointer)
	{
		delegate = static_cast<ClassDelegate>(functionPointer);
	}

	ReturnType operator()(void* object,Parameters ...params)
	{
		return static_cast<ClassType*>(object)->(*delegate)(std::forward<Parameters>(params)...);
	}
};

template <typename ReturnType, typename ...Parameters>
struct LambdaFunction {

	typedef std::function<ReturnType(Parameters...)> LambdaDelegate;

	LambdaDelegate delegate;

	void operator=(void* functionPointer)
	{
		delegate = static_cast<LambdaDelegate>(functionPointer);
	}

	ReturnType operator()(Parameters ...params)
	{
		return (*delegate)(std::forward<Parameters>(params)...);
	}
};
