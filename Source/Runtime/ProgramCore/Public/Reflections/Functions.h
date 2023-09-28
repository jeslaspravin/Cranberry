/*!
 * \file Functions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#define USE_STDFUNC_FOR_LAMDA 0

#if USE_STDFUNC_FOR_LAMDA
#include <functional>
#else
#include <type_traits>
#endif

template <typename ReturnType, typename... Parameters>
struct Function
{
    typedef ReturnType (*StaticDelegate)(Parameters...);

    StaticDelegate staticDelegate = nullptr;

    // Class default constructors and operators
    Function() = default;
    Function(const Function &otherFuncPtr) = default;
    Function &operator= (const Function &otherFuncPtr) = default;
    Function(Function &&otherFuncPtr) noexcept
        : staticDelegate(std::move(otherFuncPtr.staticDelegate))
    {
        otherFuncPtr.staticDelegate = nullptr;
    }
    Function &operator= (Function &&otherFuncPtr) noexcept
    {
        staticDelegate = std::move(otherFuncPtr.staticDelegate);
        otherFuncPtr.staticDelegate = nullptr;
        return *this;
    }
    bool operator== (const Function &otherFuncPtr) const { return staticDelegate == otherFuncPtr.staticDelegate; }
    // End class default constructors and operators

    CONST_EXPR Function(const StaticDelegate &functionPointer) noexcept
        : staticDelegate(functionPointer)
    {}

    CONST_EXPR Function &operator= (const StaticDelegate &functionPointer) noexcept
    {
        staticDelegate = functionPointer;
        return *this;
    }

    // Not using perfect forwarding as it not necessary for function ptr calls
    ReturnType operator() (Parameters... params) const noexcept { return (*staticDelegate)(std::forward<Parameters>(params)...); }

    operator bool () const { return staticDelegate != nullptr; }
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
    ClassFunction(const ClassFunction &otherFuncPtr) = default;
    ClassFunction &operator= (const ClassFunction &otherFuncPtr) = default;
    ClassFunction(ClassFunction &&otherFuncPtr) noexcept
        : classDelegate(std::move(otherFuncPtr.classDelegate))
    {
        otherFuncPtr.classDelegate = nullptr;
    }
    ClassFunction &operator= (ClassFunction &&otherFuncPtr) noexcept
    {
        classDelegate = std::move(otherFuncPtr.classDelegate);
        otherFuncPtr.classDelegate = nullptr;
        return *this;
    }
    bool operator== (const ClassFunction &otherFuncPtr) const { return classDelegate == otherFuncPtr.classDelegate; }
    // End class default constructors and operators

    CONST_EXPR ClassFunction(const ClassDelegate &functionPointer) noexcept
        : classDelegate(functionPointer)
    {}

    CONST_EXPR ClassFunction &operator= (const ClassDelegate &functionPointer) noexcept
    {
        classDelegate = functionPointer;
        return *this;
    }

    ReturnType operator() (ClassType *object, Parameters... params) const noexcept
    {
        return (object->*classDelegate)(std::forward<Parameters>(params)...);
    }

    ReturnType operator() (ClassType &object, Parameters... params) const noexcept
    {
        return (object.*classDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool () const { return classDelegate != nullptr; }
};

template <typename ClassType, typename ReturnType, typename... Parameters>
struct ClassFunction<true, ClassType, ReturnType, Parameters...>
{
    typedef ReturnType (ClassType::*ClassDelegate)(Parameters...) const;

    ClassDelegate classDelegate = nullptr;

    // Class default constructors and operators
    ClassFunction() = default;
    ClassFunction(const ClassFunction &otherFuncPtr) = default;
    ClassFunction &operator= (const ClassFunction &otherFuncPtr) = default;
    ClassFunction(ClassFunction &&otherFuncPtr) noexcept
        : classDelegate(std::move(otherFuncPtr.classDelegate))
    {
        otherFuncPtr.classDelegate = nullptr;
    }
    ClassFunction &operator= (ClassFunction &&otherFuncPtr) noexcept
    {
        classDelegate = std::move(otherFuncPtr.classDelegate);
        otherFuncPtr.classDelegate = nullptr;
        return *this;
    }
    bool operator== (const ClassFunction &otherFuncPtr) const { return classDelegate == otherFuncPtr.classDelegate; }
    // End class default constructors and operators

    CONST_EXPR ClassFunction(const ClassDelegate &functionPointer) noexcept
        : classDelegate(functionPointer)
    {}

    CONST_EXPR ClassFunction &operator= (const ClassDelegate &functionPointer)
    {
        classDelegate = functionPointer;
        return *this;
    }

    ReturnType operator() (const ClassType *object, Parameters... params) const noexcept
    {
        return (object->*classDelegate)(std::forward<Parameters>(params)...);
    }

    ReturnType operator() (const ClassType &object, Parameters... params) const noexcept
    {
        return (object.*classDelegate)(std::forward<Parameters>(params)...);
    }

    operator bool () const { return classDelegate != nullptr; }
};

#if USE_STDFUNC_FOR_LAMDA
template <typename ReturnType, typename... Parameters>
struct LambdaFunctionUsingStdFunction
{
    typedef std::function<ReturnType(Parameters...)> LambdaDelegate;

    template <typename Callable, typename Type = void>
    using IsCallable = std::enable_if_t<
        std::conjunction_v<
            std::negation<std::is_same<std::decay_t<Callable>, LambdaFunctionUsingStdFunction>>,
            std::negation<std::is_same<std::decay_t<Callable>, LambdaDelegate>>, std::is_invocable_r<ReturnType, Callable, Parameters...>>,
        Type>;

    LambdaDelegate lambdaDelegate = nullptr;

    // Class default constructors and operators
    LambdaFunctionUsingStdFunction() = default;
    LambdaFunctionUsingStdFunction(const LambdaFunctionUsingStdFunction &otherFuncPtr) = default;
    LambdaFunctionUsingStdFunction(LambdaFunctionUsingStdFunction &&otherFuncPtr) noexcept
        : lambdaDelegate(std::move(otherFuncPtr.lambdaDelegate))
    {
        otherFuncPtr.lambdaDelegate = nullptr;
    }

    LambdaFunctionUsingStdFunction &operator= (const LambdaFunctionUsingStdFunction &otherFuncPtr) = default;
    LambdaFunctionUsingStdFunction &operator= (LambdaFunctionUsingStdFunction &&otherFuncPtr) noexcept
    {
        lambdaDelegate = std::move(otherFuncPtr.lambdaDelegate);
        otherFuncPtr.lambdaDelegate = nullptr;
        return *this;
    }

    bool operator== (const LambdaFunctionUsingStdFunction &otherFuncPtr) const noexcept
    {
        return lambdaDelegate == otherFuncPtr.lambdaDelegate;
    }
    // End class default constructors and operators

    template <typename Callable, IsCallable<Callable, int> = 0>
    CONST_EXPR LambdaFunctionUsingStdFunction(Callable &&lambda) noexcept
        : lambdaDelegate(std::forward<decltype(lambda)>(lambda))
    {}

    LambdaFunctionUsingStdFunction(const LambdaDelegate &functionPointer) noexcept
        : lambdaDelegate(functionPointer)
    {}

    LambdaFunctionUsingStdFunction(LambdaDelegate &&functionPointer) noexcept
        : lambdaDelegate(std::forward<LambdaDelegate>(functionPointer))
    {}

    LambdaFunctionUsingStdFunction &operator= (const LambdaDelegate &functionPointer) noexcept
    {
        lambdaDelegate = functionPointer;
        return *this;
    }

    LambdaFunctionUsingStdFunction &operator= (LambdaDelegate &&functionPointer) noexcept
    {
        lambdaDelegate = std::forward<LambdaDelegate>(functionPointer);
        return *this;
    }

    ReturnType operator() (Parameters... params) const noexcept { return lambdaDelegate(std::forward<Parameters>(params)...); }

    operator bool () const { return bool(lambdaDelegate); }
};

template <typename ReturnType, typename... Parameters>
using LambdaFunction = LambdaFunctionUsingStdFunction<ReturnType, Parameters...>;

#else // USE_STDFUNC_FOR_LAMDA

template <typename ReturnType, typename... Parameters>
struct CapturedFunctor
{
    template <typename Callable, typename Type = void>
    using IsCallableLambda = std::enable_if_t<
        std::conjunction_v<
            std::negation<std::is_same<std::decay_t<Callable>, CapturedFunctor>>, std::is_invocable_r<ReturnType, Callable, Parameters...>>,
        Type>;

    struct LambdaFunctionCapInterface
    {
        using CopyFunc = void (*)(CapturedFunctor &, const CapturedFunctor &) noexcept;
        using MoveFunc = void (*)(CapturedFunctor &, CapturedFunctor &&) noexcept;
        using DestructFunc = void (*)(CapturedFunctor &) noexcept;
        using TrampolineFunc = ReturnType (*)(const CapturedFunctor &, Parameters...) noexcept;

        CopyFunc copyFn = nullptr;
        MoveFunc moveFn = nullptr;
        DestructFunc destructFn = nullptr;
        TrampolineFunc trampolineFn = nullptr;
    };

    /**
     * Actual data
     */
    constexpr static const SizeT MAX_INLINED_BYTES = 128;
    union
    {
        alignas(16) uint8 data[MAX_INLINED_BYTES] = {};
        struct
        {
            UPtrInt nullptrs[(MAX_INLINED_BYTES / sizeof(UPtrInt)) - 1];
            void *dataPtr;
        } heapAlloc;
    };
    const LambdaFunctionCapInterface *captureIxx = nullptr;

    // Class default constructors and operators
    CapturedFunctor() = default;
    CapturedFunctor(const CapturedFunctor &otherFuncPtr) noexcept
        : captureIxx(otherFuncPtr.captureIxx)
    {
        (*captureIxx->copyFn)(*this, otherFuncPtr);
    }
    CapturedFunctor(CapturedFunctor &&otherFuncPtr) noexcept
        : captureIxx(otherFuncPtr.captureIxx)
    {
        (*captureIxx->moveFn)(*this, std::forward<CapturedFunctor>(otherFuncPtr));
        otherFuncPtr.captureIxx = nullptr;
    }

    CapturedFunctor &operator= (const CapturedFunctor &otherFuncPtr) noexcept
    {
        captureIxx = otherFuncPtr.captureIxx;
        (*captureIxx->copyFn)(*this, otherFuncPtr);
        return *this;
    }
    CapturedFunctor &operator= (CapturedFunctor &&otherFuncPtr) noexcept
    {
        captureIxx = otherFuncPtr.captureIxx;
        (*captureIxx->moveFn)(*this, std::forward<CapturedFunctor>(otherFuncPtr));

        otherFuncPtr.captureIxx = nullptr;

        return *this;
    }

    ~CapturedFunctor() noexcept
    {
        if (captureIxx)
        {
            (*captureIxx->destructFn)(*this);
            captureIxx = nullptr;
        }
    }

    bool operator== (const CapturedFunctor &otherFuncPtr) const noexcept
    {
        // We could may be compare data's memory using memcmp()
        return captureIxx == otherFuncPtr.captureIxx;
    }
    // End class default constructors and operators
    ReturnType operator() (Parameters... params) const noexcept
    {
        return (*captureIxx->trampolineFn)(*this, std::forward<Parameters>(params)...);
    }

    operator bool () const { return captureIxx; }

    // Handling inline or heap allocated lambda data

    template <typename CallableType>
    struct IsLambdaInlineable : std::conditional_t<sizeof(CallableType) <= MAX_INLINED_BYTES, std::true_type, std::false_type>
    {};

    template <typename CallableType>
    class LambdaFunctionCaptureImpl;

    template <typename CallableType>
    requires (IsLambdaInlineable<CallableType>::value)
    class LambdaFunctionCaptureImpl<CallableType>
    {
    public:
        static void copy(CapturedFunctor &copyTo, const CapturedFunctor &copyFrom) noexcept
        {
            construct(copyTo, *reinterpret_cast<const CallableType *>(&copyFrom.data[0]));
        }
        static void move(CapturedFunctor &moveTo, CapturedFunctor &&moveFrom) noexcept
        {
            construct(moveTo, std::move(*reinterpret_cast<CallableType *>(&moveFrom.data[0])));
        }
        static void destruct(CapturedFunctor &functor) noexcept { reinterpret_cast<CallableType *>(&functor.data[0])->~CallableType(); }

        template <typename Callable>
        static void construct(CapturedFunctor &functor, Callable &&func) noexcept
        {
            new (functor.data) CallableType(std::forward<Callable>(func));
        }
        static ReturnType invoke(const CapturedFunctor &thisFunctor, Parameters... params) noexcept
        {
            return (*reinterpret_cast<CallableType *>(const_cast<uint8 *>(&thisFunctor.data[0])))(std::forward<Parameters>(params)...);
        }
    };
    template <typename CallableType>
    requires (!IsLambdaInlineable<CallableType>::value)
    class LambdaFunctionCaptureImpl<CallableType>
    {
    public:
        static void copy(CapturedFunctor &copyTo, const CapturedFunctor &copyFrom) noexcept
        {
            construct(copyTo, *reinterpret_cast<CallableType *>(copyFrom.heapAlloc.dataPtr));
        }
        static void move(CapturedFunctor &moveTo, CapturedFunctor &&moveFrom) noexcept
        {
            if (moveTo.heapAlloc.dataPtr)
            {
                destruct(moveTo);
            }
            moveTo.heapAlloc.dataPtr = moveFrom.heapAlloc.dataPtr;
            moveFrom.heapAlloc.dataPtr = nullptr;
        }
        static void destruct(CapturedFunctor &functor) noexcept
        {
            delete reinterpret_cast<CallableType *>(functor.heapAlloc.dataPtr);
            functor.heapAlloc.dataPtr = nullptr;
        }

        template <typename Callable>
        static void construct(CapturedFunctor &functor, Callable &&func) noexcept
        {
            if (functor.heapAlloc.dataPtr)
            {
                new (functor.heapAlloc.dataPtr) CallableType(std::forward<Callable>(func));
            }
            else
            {
                functor.heapAlloc.dataPtr = new CallableType(std::forward<Callable>(func));
            }
        }
        static ReturnType invoke(const CapturedFunctor &thisFunctor, Parameters... params) noexcept
        {
            return (*reinterpret_cast<CallableType *>(thisFunctor.heapAlloc.dataPtr))(std::forward<Parameters>(params)...);
        }
    };

    template <typename Callable>
    CapturedFunctor(Callable &&func) noexcept
        : captureIxx(nullptr)
    {
        this->operator= (std::forward<Callable>(func));
    }

    template <typename Callable, typename CallableType = std::remove_cvref_t<Callable>, IsCallableLambda<Callable, int> = 0>
    CapturedFunctor &operator= (Callable &&func) noexcept
    {
        using LambdaCaptureImplType = LambdaFunctionCaptureImpl<CallableType>;

        constexpr static const LambdaFunctionCapInterface lambdaCaptureIxx{ .copyFn = &LambdaCaptureImplType::copy,
                                                                            .moveFn = &LambdaCaptureImplType::move,
                                                                            .destructFn = &LambdaCaptureImplType::destruct,
                                                                            .trampolineFn = &LambdaCaptureImplType::invoke };

        captureIxx = &lambdaCaptureIxx;
        LambdaCaptureImplType::construct(*this, std::forward<Callable>(func));
        return *this;
    }
};

template <typename ReturnType, typename... Parameters>
using LambdaFunction = CapturedFunctor<ReturnType, Parameters...>;

#endif // USE_STDFUNC_FOR_LAMDA