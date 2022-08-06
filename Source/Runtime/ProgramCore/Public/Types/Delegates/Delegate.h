/*!
 * \file Delegate.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Memory/SmartPointers.h"
#include "Reflections/Functions.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#include <map>
//#include <memory_resource>
#include <tuple>
#include <type_traits>
#include <vector>

template <typename ReturnType, typename... Params>
class IDelegate
{
protected:
    IDelegate() = default;
    IDelegate(IDelegate &&other) = default;
    IDelegate(const IDelegate &other) = default;
    virtual ~IDelegate() = default;

public:
    virtual ReturnType invoke(Params... params) const = 0;
    virtual bool hasSameObject(const void *object) const { return false; }
};

template <typename... Variables>
struct FunctionExecutor
{
    using Tuple = std::tuple<Variables...>;
    using IndexSeq = std::make_index_sequence<std::tuple_size_v<Tuple>>;

    template <typename FunctionType, typename ReturnType, typename... Params>
    struct ExeHelper
    {
        template <size_t... Indices>
        ReturnType execute(const FunctionType &function, Params... params, const Tuple &varStore, std::index_sequence<Indices...>) const
        {
            return function(std::forward<Params>(params)..., std::get<Indices>(varStore)...);
        }

        template <typename ObjectType, size_t... Indices>
        ReturnType
            execute(ObjectType *object, const FunctionType &function, Params... params, const Tuple &varStore, std::index_sequence<Indices...>)
        {
            return function(object, std::forward<Params>(params)..., std::get<Indices>(varStore)...);
        }
    };

    Tuple varStore;

    FunctionExecutor() = default;
    FunctionExecutor(FunctionExecutor &&other) = default;
    FunctionExecutor(Variables... vars)
        : varStore(std::forward<Variables>(vars)...)
    {}

    template <typename ReturnType, typename FunctionType, typename... Params>
    ReturnType execute(const FunctionType &function, Params... params) const
    {
        return ExeHelper<FunctionType, ReturnType, Params...>{}.execute(function, std::forward<Params>(params)..., varStore, IndexSeq{});
    }
    template <typename ReturnType, typename FunctionType, typename ObjectType, typename... Params>
    ReturnType execute(const FunctionType &function, ObjectType *object, Params... params) const
    {
        return ExeHelper<FunctionType, ReturnType, Params...>{}.execute<ObjectType>(
            object, function, std::forward<Params>(params)..., varStore, IndexSeq{}
        );
    }
};

template <>
struct FunctionExecutor<>
{
    FunctionExecutor() = default;
    FunctionExecutor(FunctionExecutor &&other) = default;
    template <typename ReturnType, typename FunctionType, typename... Params>
    ReturnType execute(const FunctionType &function, Params... params) const
    {
        return function(std::forward<Params>(params)...);
    }
    template <typename ReturnType, typename FunctionType, typename ObjectType, typename... Params>
    ReturnType execute(const FunctionType &function, ObjectType *object, Params... params) const
    {
        return function(object, std::forward<Params>(params)...);
    }
};

template <bool IsConst, typename ObjectType, typename FuncSignature, typename... Variables>
class ObjectDelegate;

template <typename ObjectType, typename ReturnType, typename... Params, typename... Variables>
class ObjectDelegate<false, ObjectType, ReturnType(Params...), Variables...> final : public IDelegate<ReturnType, Params...>
{
public:
    using FunctionPtr = ClassFunction<false, ObjectType, ReturnType, Params..., Variables...>;
    using ThisType = ObjectDelegate<false, ObjectType, ReturnType(Params...), Variables...>;

    explicit ObjectDelegate(ObjectType *object, const FunctionPtr &functionPtr, Variables... vars)
        : delegateData(object, functionPtr)
        , executor(std::forward<Variables>(vars)...)
    {}

    ObjectDelegate(ObjectDelegate &&other)
        : delegateData(std::move(other.delegateData))
        , executor(std::move(other.executor))
    {}

    ReturnType invoke(Params... params) const override
    {
        // Has to send full Params variadic types as l-value references are getting lost
        return executor.execute<ReturnType, FunctionPtr, decltype(delegateData.objectPtr), Params...>(
            delegateData.functionPtr, delegateData.objectPtr, std::forward<Params>(params)...
        );
    }

    bool hasSameObject(const void *object) const override { return delegateData.objectPtr == object; }

private:
    struct FunctionHolder
    {
        FunctionPtr functionPtr;
        ObjectType *objectPtr;

        FunctionHolder(ObjectType *object, const FunctionPtr &functionPtr)
            : objectPtr(object)
            , functionPtr(functionPtr)
        {}

        FunctionHolder(FunctionHolder &&other)
            : objectPtr(std::move(objectPtr))
            , functionPtr(std::move(other.functionPtr))
        {
            other.objectPtr = nullptr;
            other.functionPtr = nullptr;
        }

        bool operator==(const FunctionHolder &otherType) const
        {
            return objectPtr == otherType.objectPtr && functionPtr == otherType.functionPtr;
        }
    };

    FunctionHolder delegateData;
    FunctionExecutor<Variables...> executor;
};

template <typename ObjectType, typename ReturnType, typename... Params, typename... Variables>
class ObjectDelegate<true, ObjectType, ReturnType(Params...), Variables...> final : public IDelegate<ReturnType, Params...>
{
public:
    using FunctionPtr = ClassFunction<true, ObjectType, ReturnType, Params..., Variables...>;
    using ThisType = ObjectDelegate<true, ObjectType, ReturnType(Params...), Variables...>;

    explicit ObjectDelegate(const ObjectType *object, const FunctionPtr &functionPtr, Variables... vars)
        : delegateData(object, functionPtr)
        , executor(std::forward<Variables>(vars)...)
    {}

    ObjectDelegate(ObjectDelegate &&other)
        : delegateData(std::move(other.delegateData))
        , executor(std::move(other.executor))
    {}

    ReturnType invoke(Params... params) const override
    {
        // Has to send full Params variadic types as l-value references are getting lost
        return executor.execute<ReturnType, FunctionPtr, decltype(delegateData.objectPtr), Params...>(
            delegateData.functionPtr, delegateData.objectPtr, std::forward<Params>(params)...
        );
    }

    bool hasSameObject(const void *object) const override { return delegateData.objectPtr == object; }

private:
    struct FunctionHolder
    {
        FunctionPtr functionPtr;
        const ObjectType *objectPtr;

        FunctionHolder(const ObjectType *object, const FunctionPtr &functionPtr)
            : objectPtr(object)
            , functionPtr(functionPtr)
        {}

        FunctionHolder(FunctionHolder &&other)
            : objectPtr(std::move(objectPtr))
            , functionPtr(std::move(other.functionPtr))
        {
            other.objectPtr = nullptr;
            other.functionPtr = nullptr;
        }

        bool operator==(const FunctionHolder &otherType) const
        {
            return objectPtr == otherType.objectPtr && functionPtr == otherType.functionPtr;
        }
    };

    FunctionHolder delegateData;
    FunctionExecutor<Variables...> executor;
};

template <typename FuncSignature, typename... Variables>
class StaticDelegate;

template <typename ReturnType, typename... Params, typename... Variables>
class StaticDelegate<ReturnType(Params...), Variables...> final : public IDelegate<ReturnType, Params...>
{
public:
    using FunctionPtr = Function<ReturnType, Params..., Variables...>;
    using ThisType = StaticDelegate<ReturnType, Params..., Variables...>;

    explicit StaticDelegate(const FunctionPtr &functionPtr, Variables... vars)
        : fPtr(functionPtr)
        , executor(vars...)
    {}

    StaticDelegate(StaticDelegate &&other)
        : fPtr(std::move(other.fPtr))
        , executor(std::move(other.executor))
    {
        other.fPtr = nullptr;
    }

private:
    ReturnType invoke(Params... params) const override
    {
        // Has to send full Params variadic types as l-value references are getting lost
        return executor.execute<ReturnType, FunctionPtr, Params...>(fPtr, params...);
    }

private:
    FunctionPtr fPtr;
    FunctionExecutor<Variables...> executor;
};

template <typename FuncSignature, typename... Variables>
class LambdaDelegate;

template <typename ReturnType, typename... Params, typename... Variables>
class LambdaDelegate<ReturnType(Params...), Variables...> final : public IDelegate<ReturnType, Params...>
{

public:
    using FunctionPtr = LambdaFunction<ReturnType, Params..., Variables...>;
    using ThisType = LambdaDelegate<ReturnType, Params..., Variables...>;

    explicit LambdaDelegate(const FunctionPtr &functionPtr, Variables... vars)
        : fPtr(functionPtr)
        , executor(vars...)
    {}

    explicit LambdaDelegate(FunctionPtr &&functionPtr, Variables... vars)
        : fPtr(std::forward<FunctionPtr>(functionPtr))
        , executor(vars...)
    {}

    LambdaDelegate(LambdaDelegate &&other)
        : fPtr(std::move(other.fPtr))
        , executor(std::move(other.executor))
    {
        other.fPtr = nullptr;
    }

private:
    ReturnType invoke(Params... params) const override
    {
        // Has to send full Params variadic types as l-value references are getting lost
        return executor.execute<ReturnType, FunctionPtr, Params...>(fPtr, params...);
    }

private:
    FunctionPtr fPtr;
    FunctionExecutor<Variables...> executor;
};

struct DelegateHandle
{
    int32 value = -1;

    bool isValid() const { return value >= 0; }
};

template <typename ReturnType, typename... Params>
class SingleCastDelegateBase
{
public:
    using DelegateInterface = IDelegate<ReturnType, Params...>;

    // Using this function signature to deduce return and params type to individual delegate
    typedef ReturnType FuncType(Params...);

protected:
    SharedPtr<DelegateInterface> delegatePtr;

    template <typename ObjectType, typename... Variables>
    using ObjDelegateType = ObjectDelegate<false, ObjectType, FuncType, Variables...>;
    template <typename ObjectType, typename... Variables>
    using ConstObjDelegateType = ObjectDelegate<true, ObjectType, FuncType, Variables...>;
    template <typename... Variables>
    using StaticDelegateType = StaticDelegate<FuncType, Variables...>;
    template <typename... Variables>
    using LambdaDelegateType = LambdaDelegate<FuncType, Variables...>;

public:
    template <typename ObjectType, typename... Variables>
    std::enable_if_t<std::negation_v<std::is_const<ObjectType>>, void> bindObject(
        ObjectType *object, const typename ObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction, Variables... vars
    )
    {
        delegatePtr.reset(new ObjDelegateType<ObjectType, Variables...>(object, bindingFunction, std::forward<Variables>(vars)...));
    }

    template <typename ObjectType, typename... Variables>
    void bindObject(
        const ObjectType *object, const typename ConstObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction, Variables... vars
    )
    {
        delegatePtr.reset(new ConstObjDelegateType<ObjectType, Variables...>(object, bindingFunction, std::forward<Variables>(vars)...));
    }

    template <typename... Variables>
    void bindStatic(const typename StaticDelegateType<Variables...>::FunctionPtr &bindingFunction, Variables... vars)
    {
        delegatePtr.reset(new StaticDelegateType<Variables...>(bindingFunction, std::forward<Variables>(vars)...));
    }

    template <typename... Variables>
    void bindLambda(const typename LambdaDelegateType<Variables...>::FunctionPtr &lambda, Variables... vars)
    {
        delegatePtr.reset(new LambdaDelegateType<Variables...>(lambda, std::forward<Variables>(vars)...));
    }

    template <typename LambdaType, typename... Variables>
    void bindLambda(LambdaType &&lambda, Variables... vars)
    {
        delegatePtr.reset(new LambdaDelegateType<Variables...>(
            LambdaDelegateType<Variables...>::FunctionPtr(std::forward<LambdaType &&>(lambda)), std::forward<Variables>(vars)...
        ));
    }

    void unbind() { delegatePtr.reset(); }

    bool isBound() const { return bool(delegatePtr); }
    operator bool() const { return isBound(); }

    template <typename ObjectType>
    bool isBoundTo(const ObjectType *object) const
    {
        return delegatePtr->hasSameObject(object);
    }
};

template <typename... Params>
class MultiCastDelegateBase;

//////////////////////////////////////////////////////////////////////////
// SingleCast delegate and event implementations
//////////////////////////////////////////////////////////////////////////

/* Does not handle life time of object in delegates, needs to be externally handled */
template <typename ReturnType, typename... Params>
class SingleCastDelegate : public SingleCastDelegateBase<ReturnType, Params...>
{
private:
    // Since delegatePtr need to be accessible in MultiCastDelegateBase
    friend MultiCastDelegateBase<Params...>;

    using Base = SingleCastDelegateBase<ReturnType, Params...>;
    using SingleCastDelegateBase<ReturnType, Params...>::delegatePtr;

public:
    using SingleCastDelegateBase<ReturnType, Params...>::unbind;
    ~SingleCastDelegate() { unbind(); }

    ReturnType invoke(Params... params) const { return delegatePtr->invoke(std::forward<Params>(params)...); }
    ReturnType operator()(Params... params) const { return invoke(std::forward<Params>(params)...); }

    template <typename ObjectType, typename... Variables>
    static std::enable_if_t<std::negation_v<std::is_const<ObjectType>>, SingleCastDelegate> createObject(
        ObjectType *object, const typename Base::template ObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction,
        Variables... vars
    )
    {
        SingleCastDelegate sDelegate;
        sDelegate.bindObject(object, bindingFunction, std::forward<Variables>(vars)...);
        return sDelegate;
    }

    template <typename ObjectType, typename... Variables>
    static SingleCastDelegate createObject(
        const ObjectType *object, const typename Base::template ConstObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction,
        Variables... vars
    )
    {
        SingleCastDelegate sDelegate;
        sDelegate.bindObject(object, bindingFunction, std::forward<Variables>(vars)...);
        return sDelegate;
    }

    template <typename... Variables>
    static SingleCastDelegate
        createStatic(const typename Base::template StaticDelegateType<Variables...>::FunctionPtr &bindingFunction, Variables... vars)
    {
        SingleCastDelegate sDelegate;
        sDelegate.bindStatic(bindingFunction, std::forward<Variables>(vars)...);
        return sDelegate;
    }

    template <typename LambdaType, typename... Variables>
    static SingleCastDelegate createLambda(LambdaType &&lambda, Variables... vars)
    {
        SingleCastDelegate sDelegate;
        sDelegate.bindLambda(std::forward<LambdaType &&>(lambda), std::forward<Variables>(vars)...);
        return sDelegate;
    }
};

/* Does not handle life time of object delegates needs to be externally handled */
template <typename OwnerType, typename ReturnType, typename... Params>
class SingleCastEvent : public SingleCastDelegateBase<ReturnType, Params...>
{
private:
    using SingleCastDelegateBase<ReturnType, Params...>::DelegateInterface;
    using SingleCastDelegateBase<ReturnType, Params...>::delegatePtr;

    friend OwnerType;

    ReturnType invoke(Params... params) const { return delegatePtr->invoke(std::forward<Params>(params)...); }

public:
    using SingleCastDelegateBase<ReturnType, Params...>::unbind;
    ~SingleCastEvent() { unbind(); }
};

//////////////////////////////////////////////////////////////////////////
// Multicast delegates
//////////////////////////////////////////////////////////////////////////

template <typename... Params>
class MultiCastDelegateBase
{
public:
    using SingleCastDelegateType = SingleCastDelegate<void, Params...>;
    using DelegateInterface = IDelegate<void, Params...>;

    // Using this function signature to deduce return and params type to individual delegate
    typedef void FuncType(Params...);

protected:
    std::map<int32, SharedPtr<DelegateInterface>> allDelegates;

    template <typename ObjectType, typename... Variables>
    using ObjDelegateType = ObjectDelegate<false, ObjectType, FuncType, Variables...>;
    template <typename ObjectType, typename... Variables>
    using ConstObjDelegateType = ObjectDelegate<true, ObjectType, FuncType, Variables...>;
    template <typename... Variables>
    using StaticDelegateType = StaticDelegate<FuncType, Variables...>;
    template <typename... Variables>
    using LambdaDelegateType = LambdaDelegate<FuncType, Variables...>;

    // Returns true if ID is found at end of sequence
    bool getNextId(int32 &nextId) const
    {
        int32 index = -1;
        for (std::pair<int32, SharedPtr<DelegateInterface>> indexedDelegate : allDelegates)
        {
            index++;
            if (indexedDelegate.first != index) // Map always sorts, so if an index is not equal when
                                                // equally incrementing it is not available.
            {
                nextId = index;
                return false;
            }
        }
        nextId = index + 1;
        return true;
    }

public:
    template <typename ObjectType, typename... Variables>
    std::enable_if_t<std::negation_v<std::is_const<ObjectType>>, DelegateHandle> bindObject(
        ObjectType *object, const typename ObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction, Variables... vars
    )
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(
            new ObjDelegateType<ObjectType, Variables...>(object, bindingFunction, std::forward<Variables>(vars)...)
        );
        DelegateHandle handle;
        getNextId(handle.value);
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template <typename ObjectType, typename... Variables>
    DelegateHandle bindObject(
        const ObjectType *object, const typename ConstObjDelegateType<ObjectType, Variables...>::FunctionPtr &bindingFunction, Variables... vars
    )
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(
            new ConstObjDelegateType<ObjectType, Variables...>(object, bindingFunction, std::forward<Variables>(vars)...)
        );
        DelegateHandle handle;
        getNextId(handle.value);
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template <typename... Variables>
    DelegateHandle bindStatic(const typename StaticDelegateType<Variables...>::FunctionPtr &bindingFunction, Variables... vars)
    {
        SharedPtr<DelegateInterface> ptr
            = SharedPtr<DelegateInterface>(new StaticDelegateType<Variables...>(bindingFunction, std::forward<Variables>(vars)...));
        DelegateHandle handle;
        getNextId(handle.value);
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template <typename... Variables>
    DelegateHandle bindLambda(const typename LambdaDelegateType<Variables...>::FunctionPtr &lambda, Variables... vars)
    {
        SharedPtr<DelegateInterface> ptr
            = SharedPtr<DelegateInterface>(new LambdaDelegateType<Variables...>(lambda, std::forward<Variables>(vars)...));
        DelegateHandle handle;
        getNextId(handle.value);
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template <typename LambdaType, typename... Variables>
    DelegateHandle bindLambda(LambdaType &&lambda, Variables... vars)
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(new LambdaDelegateType<Variables...>(
            LambdaDelegateType<Variables...>::FunctionPtr(std::forward<LambdaType>(lambda)), std::forward<Variables>(vars)...
        ));
        DelegateHandle handle;
        getNextId(handle.value);
        allDelegates[handle.value] = ptr;
        return handle;
    }

    // Returns list of pair of old to new DelegateHandle
    std::vector<std::pair<DelegateHandle, DelegateHandle>> bind(const MultiCastDelegateBase &fromDelegate)
    {
        std::vector<std::pair<DelegateHandle, DelegateHandle>> retHandles;

        int32 nextId;
        bool bSeqEnd = getNextId(nextId);
        for (auto fromItr = fromDelegate.allDelegates.begin(); fromItr != fromDelegate.allDelegates.end(); ++fromItr)
        {
            DelegateHandle handle;
            handle.value = nextId;
            if (bSeqEnd)
            {
                nextId++;
            }
            else
            {
                bSeqEnd = getNextId(nextId);
            }
            retHandles.emplace_back(std::pair<DelegateHandle, DelegateHandle>{ { fromItr->first }, handle });
            allDelegates[handle.value] = fromItr->second;
        }
        return retHandles;
    }

    DelegateHandle bind(const SingleCastDelegateType &fromDelegate)
    {
        int32 nextId;
        getNextId(nextId);

        DelegateHandle handle;
        handle.value = nextId;

        allDelegates[handle.value] = fromDelegate.delegatePtr;
        return handle;
    }

    void unbind(const DelegateHandle &handle)
    {
        if (handle.isValid())
        {
            auto itr = allDelegates.find(handle.value);
            if (itr != allDelegates.end())
            {
                allDelegates.erase(itr);
            }
        }
    }

    template <typename ObjectType>
    void unbindAll(const ObjectType *object)
    {
        for (auto itr = allDelegates.begin(); itr != allDelegates.end();)
        {
            if (itr->second->hasSameObject(object))
            {
                allDelegates.erase(itr++);
            }
            else
            {
                ++itr;
            }
        }
    }

    bool isBound() const { return !allDelegates.empty(); }
    operator bool() const { return isBound(); }

    void clear();
};

template <typename... Params>
void MultiCastDelegateBase<Params...>::clear()
{
    allDelegates.clear();
}

//////////////////////////////////////////////////////////////////////////
// Delegate and Event implementations
//////////////////////////////////////////////////////////////////////////

template <typename... Params>
FORCE_INLINE void invokeHelper(const std::map<int32, SharedPtr<IDelegate<void, Params...>>> &allDelegates, Params... params)
{
    if (allDelegates.empty())
        return;

    int32 buffer[128];
    // <memory_resource> is huge header so not using it in delegate
    // TODO(Jeslas) : Improve this with some form of inline allocator in the future
    // std::pmr::monotonic_buffer_resource bufferRes(buffer, ARRAY_LENGTH(buffer) * sizeof(uint32));
    // std::pmr::vector<int32> delegateIndices(&bufferRes);
    int32 *delegateIndices = buffer;
    if (allDelegates.size() > ARRAY_LENGTH(buffer))
    {
        delegateIndices = new int32[allDelegates.size()];
    }

    uint32 delegateIndicesIdx = 0;
    for (const std::pair<const int32, SharedPtr<IDelegate<void, Params...>>> &delegateInterface : allDelegates)
    {
        delegateIndices[delegateIndicesIdx] = delegateInterface.first;
        delegateIndicesIdx++;
    }

    uint32 allDelegatesCount = uint32(allDelegates.size());
    for (delegateIndicesIdx = 0; delegateIndicesIdx < allDelegatesCount; ++delegateIndicesIdx)
    {
        auto itr = allDelegates.find(delegateIndices[delegateIndicesIdx]);
        if (itr != allDelegates.cend())
        {
            itr->second->invoke(std::forward<Params>(params)...);
        }
    }

    if (allDelegatesCount > ARRAY_LENGTH(buffer))
    {
        delete[] delegateIndices;
    }
}

/* Does not handle life time of object in delegates, needs to be externally handled */
template <typename... Params>
class Delegate : public MultiCastDelegateBase<Params...>
{
private:
    using typename MultiCastDelegateBase<Params...>::DelegateInterface;
    using MultiCastDelegateBase<Params...>::allDelegates;

public:
    using MultiCastDelegateBase<Params...>::clear;
    ~Delegate();

    void invoke(Params... params) const { invokeHelper<Params...>(allDelegates, std::forward<Params>(params)...); }
    void operator()(Params... params) const { invoke(std::forward<Params>(params)...); }
};

using SimpleDelegate = Delegate<>;

template <typename... Params>
Delegate<Params...>::~Delegate()
{
    clear();
}

/* Does not handle life time of object delegates needs to be externally handled */
template <typename OwnerType, typename... Params>
class Event : public MultiCastDelegateBase<Params...>
{
private:
    using typename MultiCastDelegateBase<Params...>::DelegateInterface;
    using MultiCastDelegateBase<Params...>::allDelegates;

    friend OwnerType;

    void invoke(Params... params) const { invokeHelper<Params...>(allDelegates, std::forward<Params>(params)...); }

public:
    using MultiCastDelegateBase<Params...>::clear;
    ~Event();
};

template <typename OwnerType, typename... Params>
Event<OwnerType, Params...>::~Event()
{
    clear();
}

using SimpleSingleCastDelegate = SingleCastDelegate<void>;
using SimpleDelegate = Delegate<>;
template <typename OwnerType>
using SimpleSingleCastEvent = SingleCastEvent<OwnerType, void>;
template <typename OwnerType>
using SimpleEvent = Event<OwnerType>;