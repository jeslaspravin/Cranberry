#pragma once

#include <functional>

template <typename ReturnType, typename... Parameters>
struct Function {

    typedef ReturnType(*StaticDelegate)(Parameters...);

    StaticDelegate staticDelegate = nullptr;

    virtual void operator=(void* functionPointer)
    {
        staticDelegate = static_cast<StaticDelegate>(functionPointer);
    }

    virtual ReturnType operator()(Parameters... params)
    {
        return (*staticDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool()
    {
        return staticDelegate != nullptr;
    }
};

template <typename ClassType,typename ReturnType, typename... Parameters>
struct ClassFunction {
    typedef ReturnType (ClassType::*ClassDelegate)(Parameters...);

    ClassDelegate classDelegate = nullptr;

    void operator=(const ClassDelegate& functionPointer)
    {
        classDelegate = functionPointer;
    }

    ReturnType operator()(ClassType* object,Parameters... params)
    {
        return (object->*classDelegate)(std::forward<Parameters>(params)...);
    }

    ReturnType operator()(ClassType& object, Parameters... params)
    {
        return (object.*classDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool()
    {
        return classDelegate != nullptr;
    }
};

template <typename ReturnType, typename... Parameters>
struct LambdaFunction {

    typedef std::function<ReturnType(Parameters...)> LambdaDelegate;

    LambdaDelegate lambdaDelegate = nullptr;

    void operator=(void* functionPointer)
    {
        lambdaDelegate = static_cast<LambdaDelegate>(functionPointer);
    }

    ReturnType operator()(Parameters... params)
    {
        return (*lambdaDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool()
    {
        return lambdaDelegate != nullptr;
    }
};
