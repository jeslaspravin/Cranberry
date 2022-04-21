/*!
 * \file Functions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"

#include <functional>

template <typename ReturnType, typename... Parameters>
struct Function
{

    typedef ReturnType (*StaticDelegate)(Parameters...);

    StaticDelegate staticDelegate = nullptr;

    // Class default constructors and operators
    Function() = default;
    Function(const Function &otherFuncPtr)
        : staticDelegate(otherFuncPtr.staticDelegate)
    {}
    void operator=(const Function &otherFuncPtr) { staticDelegate = otherFuncPtr.staticDelegate; }
    Function(Function &&otherFuncPtr)
        : staticDelegate(std::move(otherFuncPtr.staticDelegate))
    {
        otherFuncPtr.staticDelegate = nullptr;
    }
    void operator=(Function &&otherFuncPtr)
    {
        staticDelegate = std::move(otherFuncPtr.staticDelegate);
        otherFuncPtr.staticDelegate = nullptr;
    }
    bool operator==(const Function &otherFuncPtr) { return staticDelegate == otherFuncPtr.staticDelegate; }
    // End class default constructors and operators

    CONST_EXPR Function(const StaticDelegate &functionPointer)
        : staticDelegate(functionPointer)
    {}

    CONST_EXPR void operator=(const StaticDelegate &functionPointer) { staticDelegate = functionPointer; }

    // Not using perfect forwarding as it not necessary for function ptr calls
    ReturnType operator()(Parameters... params) const { return (*staticDelegate)(std::forward<Parameters>(params)...); }

    operator bool() const { return staticDelegate != nullptr; }
};

template <bool IsConst, typename ClassType, typename ReturnType, typename... Parameters>
struct ClassFunction;

template <typename ClassType, typename ReturnType, typename... Parameters>
struct ClassFunction<false, ClassType, ReturnType, Parameters...>
{
    typedef ReturnType (ClassType::*ClassDelegate)(Parameters...);

    ClassDelegate classDelegate = nullptr;

    // Class default constructors and operators
    ClassFunction() = default;
    ClassFunction(const ClassFunction &otherFuncPtr)
        : classDelegate(otherFuncPtr.classDelegate)
    {}
    void operator=(const ClassFunction &otherFuncPtr) { classDelegate = otherFuncPtr.classDelegate; }
    ClassFunction(ClassFunction &&otherFuncPtr)
        : classDelegate(std::move(otherFuncPtr.classDelegate))
    {
        otherFuncPtr.classDelegate = nullptr;
    }
    void operator=(ClassFunction &&otherFuncPtr)
    {
        classDelegate = std::move(otherFuncPtr.classDelegate);
        otherFuncPtr.classDelegate = nullptr;
    }
    bool operator==(const ClassFunction &otherFuncPtr) { return classDelegate == otherFuncPtr.classDelegate; }
    // End class default constructors and operators

    CONST_EXPR ClassFunction(const ClassDelegate &functionPointer)
        : classDelegate(functionPointer)
    {}

    CONST_EXPR void operator=(const ClassDelegate &functionPointer) { classDelegate = functionPointer; }

    ReturnType operator()(ClassType *object, Parameters... params) const
    {
        return (object->*classDelegate)(std::forward<Parameters>(params)...);
    }

    ReturnType operator()(ClassType &object, Parameters... params) const
    {
        return (object.*classDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool() const { return classDelegate != nullptr; }
};

template <typename ClassType, typename ReturnType, typename... Parameters>
struct ClassFunction<true, ClassType, ReturnType, Parameters...>
{
    typedef ReturnType (ClassType::*ClassDelegate)(Parameters...) const;

    ClassDelegate classDelegate = nullptr;

    // Class default constructors and operators
    ClassFunction() = default;
    ClassFunction(const ClassFunction &otherFuncPtr)
        : classDelegate(otherFuncPtr.classDelegate)
    {}
    void operator=(const ClassFunction &otherFuncPtr) { classDelegate = otherFuncPtr.classDelegate; }
    ClassFunction(ClassFunction &&otherFuncPtr)
        : classDelegate(std::move(otherFuncPtr.classDelegate))
    {
        otherFuncPtr.classDelegate = nullptr;
    }
    void operator=(ClassFunction &&otherFuncPtr)
    {
        classDelegate = std::move(otherFuncPtr.classDelegate);
        otherFuncPtr.classDelegate = nullptr;
    }
    bool operator==(const ClassFunction &otherFuncPtr) { return classDelegate == otherFuncPtr.classDelegate; }
    // End class default constructors and operators

    CONST_EXPR ClassFunction(const ClassDelegate &functionPointer)
        : classDelegate(functionPointer)
    {}

    CONST_EXPR void operator=(const ClassDelegate &functionPointer) { classDelegate = functionPointer; }

    ReturnType operator()(const ClassType *object, Parameters... params) const
    {
        return (object->*classDelegate)(std::forward<Parameters>(params)...);
    }

    ReturnType operator()(const ClassType &object, Parameters... params) const
    {
        return (object.*classDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool() const { return classDelegate != nullptr; }
};

template <typename ReturnType, typename... Parameters>
struct LambdaFunction
{
    typedef std::function<ReturnType(Parameters...)> LambdaDelegate;

    template <typename Callable, typename Type = void>
    using IsCallable = std::enable_if_t<
        std::conjunction_v<
            std::negation<std::is_same<std::decay_t<Callable>, LambdaFunction>>,
            std::negation<std::is_same<std::decay_t<Callable>, LambdaDelegate>>, std::is_invocable_r<ReturnType, Callable, Parameters...>>,
        Type>;

    LambdaDelegate lambdaDelegate = nullptr;

    // Class default constructors and operators
    LambdaFunction() = default;
    LambdaFunction(const LambdaFunction &otherFuncPtr)
        : lambdaDelegate(otherFuncPtr.lambdaDelegate)
    {}
    LambdaFunction(LambdaFunction &&otherFuncPtr)
        : lambdaDelegate(std::move(otherFuncPtr.lambdaDelegate))
    {
        otherFuncPtr.lambdaDelegate = nullptr;
    }

    void operator=(const LambdaFunction &otherFuncPtr) { lambdaDelegate = otherFuncPtr.lambdaDelegate; }
    void operator=(LambdaFunction &&otherFuncPtr)
    {
        lambdaDelegate = std::move(otherFuncPtr.lambdaDelegate);
        otherFuncPtr.lambdaDelegate = nullptr;
    }

    bool operator==(const LambdaFunction &otherFuncPtr) { return lambdaDelegate == otherFuncPtr.lambdaDelegate; }
    // End class default constructors and operators

    template <typename Callable, IsCallable<Callable, int> = 0>
    CONST_EXPR LambdaFunction(Callable &&lambda)
        : lambdaDelegate(std::forward<decltype(lambda)>(lambda))
    {}

    LambdaFunction(const LambdaDelegate &functionPointer)
        : lambdaDelegate(functionPointer)
    {}

    LambdaFunction(LambdaDelegate &&functionPointer)
        : lambdaDelegate(std::forward<LambdaDelegate>(functionPointer))
    {}

    void operator=(const LambdaDelegate &functionPointer) { lambdaDelegate = functionPointer; }

    void operator=(LambdaDelegate &&functionPointer) { lambdaDelegate = std::forward<LambdaDelegate>(functionPointer); }

    ReturnType operator()(Parameters... params) const { return lambdaDelegate(std::forward<Parameters>(params)...); }

    operator bool() const { return bool(lambdaDelegate); }
};

// Working impl for lightweight lambda alternative using trampoline redirection
// #TODO(Jeslas) : Improve and clean up, Must be renamed to LambdaFunction after matching all signatures
// #WARNING Not read for usage
template <typename RetType, typename... Params>
struct CapturedFunctor
{
    unsigned char *data;
    using TrampolineFunc = RetType (*)(const CapturedFunctor &, Params...);
    TrampolineFunc trampolineFunc;

    template <typename Callable, typename CallableType = std::remove_cvref_t<Callable>>
    CapturedFunctor(Callable &&func)
        : data(nullptr)
        , trampolineFunc(nullptr)
    {
        if (data)
            delete[] data;

        data = new unsigned char[sizeof(CallableType)];
        memcpy(data, &func, sizeof(CallableType));

        struct Trampoline
        {
            static RetType invoke(const CapturedFunctor &thisFunctor, Params... params)
            {
                (*reinterpret_cast<CallableType *>(thisFunctor.data))(std::forward<Params>(params)...);
            }
        };
        trampolineFunc = &Trampoline::invoke;
    }
    ~CapturedFunctor()
    {
        delete[] data;
        data = nullptr;
    }

    RetType operator()(Params... params) const { return (*trampolineFunc)(*this, std::forward<Params>(params)...); }
};