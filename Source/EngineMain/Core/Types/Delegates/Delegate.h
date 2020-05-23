#pragma once

#include "../Functions.h"
#include "../../Memory/SmartPointers.h"
#include "../../Platform/PlatformTypes.h"

#include <list>
#include <map>
#include <type_traits>

// TODO(Jeslas) revisit all these later

template <typename... Params>
class MultiCastDelegateBase;

template<typename ReturnType, typename... Params>
class IDelegate
{
protected:
    IDelegate() = default;
    IDelegate(IDelegate&& other) = default;
    IDelegate(const IDelegate& other) = default;
    virtual ~IDelegate() = default;
public:
    virtual ReturnType invoke(Params... params) const = 0;
    virtual bool hasSameObject(const void* object) const { return false; }
};

template<typename... Variables>
struct FunctionExecutor;
template<> struct FunctionExecutor<>
{
    FunctionExecutor() = default;
    FunctionExecutor(FunctionExecutor&& other) = default;
    bool operator==(const FunctionExecutor& other) { return true; }
    template<typename ReturnType, typename FunctionType, typename... Params> ReturnType execute(const FunctionType& function, Params ...params) const { return function(std::forward<Params>(params)...); }
    template<typename ReturnType, typename FunctionType, typename ObjectType, typename... Params> ReturnType execute(const FunctionType& function, ObjectType* object, Params ...params) const { return function(object, std::forward<Params>(params)...); }
};
template<typename Var1> struct FunctionExecutor<Var1>
{
    Var1 var1;
    FunctionExecutor(Var1 variable1) : var1(variable1){}
    FunctionExecutor(FunctionExecutor&& other) : var1(std::move(other.var1)) {}
    //bool operator==(const FunctionExecutor& other) { return var1 == other.var1; }
    template<typename ReturnType, typename FunctionType, typename... Params> ReturnType execute(const FunctionType& function, Params ...params) const { return function(std::forward<Params>(params)..., var1); }
    template<typename ReturnType, typename FunctionType, typename ObjectType, typename... Params> ReturnType execute(const FunctionType& function, ObjectType* object, Params ...params) const { return function(object, std::forward<Params>(params)..., var1); }
};
template<typename Var1, typename Var2> struct FunctionExecutor<Var1,Var2>
{
    Var1 var1;
    Var2 var2;
    FunctionExecutor(Var1 variable1, Var2 variable2) : var1(variable1), var2(variable2) {}
    FunctionExecutor(FunctionExecutor&& other) : var1(std::move(other.var1)), var2(std::move(other.var2)) {}
    //bool operator==(const FunctionExecutor& other) { return var1 == other.var1 && var2 == other.var2; }
    template<typename ReturnType, typename FunctionType, typename... Params> ReturnType execute(const FunctionType& function, Params ...params) const { return function(std::forward<Params>(params)..., var1, var2); }
    template<typename ReturnType, typename FunctionType, typename ObjectType, typename... Params> ReturnType execute(const FunctionType& function, ObjectType* object, Params ...params) const { return function(object, std::forward<Params>(params)..., var1, var2); }
};
template<typename Var1, typename Var2, typename Var3> struct FunctionExecutor<Var1,Var2,Var3>
{
    Var1 var1;
    Var2 var2;
    Var3 var3;
    FunctionExecutor(Var1 variable1, Var2 variable2, Var3 variable3) : var1(variable1), var2(variable2), var3(variable3) {}
    FunctionExecutor(FunctionExecutor&& other) : var1(std::move(other.var1)), var2(std::move(other.var2)), var3(std::move(other.var3)) {}
    //bool operator==(const FunctionExecutor& other) { return var1 == other.var1 && var2 == other.var2 && var3 == other.var3; }
    template<typename ReturnType, typename FunctionType, typename... Params> ReturnType execute(const FunctionType& function, Params ...params) const { return function(std::forward<Params>(params)..., var1, var2, var3); }
    template<typename ReturnType, typename FunctionType, typename ObjectType, typename... Params> ReturnType execute(const FunctionType& function, ObjectType* object, Params ...params) const { return function(object, std::forward<Params>(params)..., var1, var2, var3); }
};
template<typename Var1, typename Var2, typename Var3, typename Var4> struct FunctionExecutor<Var1, Var2, Var3, Var4>
{
    Var1 var1;
    Var2 var2;
    Var3 var3;
    Var4 var4;
    FunctionExecutor(Var1 variable1, Var2 variable2, Var3 variable3, Var4 variable4) : var1(variable1), var2(variable2), var3(variable3), var4(variable4) {}
    FunctionExecutor(FunctionExecutor&& other) : var1(std::move(other.var1)), var2(std::move(other.var2)), var3(std::move(other.var3)), var4(std::move(other.var4)) {}
    //bool operator==(const FunctionExecutor& other) { return var1 == other.var1 && var2 == other.var2 && var3 == other.var3 && var4 == other.var4; }
    template<typename ReturnType, typename FunctionType, typename... Params> ReturnType execute(const FunctionType& function, Params ...params) const { return function(std::forward<Params>(params)..., var1, var2, var3, var4); }
    template<typename ReturnType, typename FunctionType, typename ObjectType, typename... Params> ReturnType execute(const FunctionType& function, ObjectType* object, Params ...params) const { return function(object, std::forward<Params>(params)..., var1, var2, var3, var4); }
};

template <bool IsConst,typename ObjectType,typename FuncSignature, typename... Variables>
class ObjectDelegate;

template <typename ObjectType,typename ReturnType,typename... Params, typename... Variables>
class ObjectDelegate<false,ObjectType,ReturnType(Params...),Variables...> final : public IDelegate<ReturnType, Params...>
{
public:
    using FunctionPtr = ClassFunction<false, ObjectType, ReturnType, Params..., Variables...>;
    using ThisType = ObjectDelegate<false, ObjectType, ReturnType(Params...), Variables...>;

    explicit ObjectDelegate(ObjectType* object,const FunctionPtr& functionPtr, Variables ...vars)
        : delegateData(object, functionPtr)
        , executor(std::forward<Variables>(vars)...)
    {}

    ObjectDelegate(ObjectDelegate&& other)
        : delegateData(std::move(other.delegateData))
        , executor(std::move(other.executor))
    {}

    ReturnType invoke(Params... params) const override
    {
        return executor.execute<ReturnType>(delegateData.functionPtr, delegateData.objectPtr, std::forward<Params>(params)...);
    }

    virtual bool hasSameObject(const void* object) const override
    {
        return delegateData.objectPtr == object;
    }

private:
	struct FunctionHolder
	{
		FunctionPtr functionPtr;
		ObjectType* objectPtr;

		FunctionHolder(ObjectType* object,const FunctionPtr& functionPtr)
			: objectPtr(object)
			, functionPtr(functionPtr)
		{}

        FunctionHolder(FunctionHolder&& other)
            : objectPtr(std::move(objectPtr))
			, functionPtr(std::move(other.functionPtr))
        {
            other.objectPtr = nullptr;
            other.functionPtr = nullptr;
        }

		bool operator==(const FunctionHolder& otherType) const
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

    explicit ObjectDelegate(const ObjectType* object, const FunctionPtr& functionPtr, Variables ...vars)
        : delegateData(object, functionPtr)
        , executor(std::forward<Variables>(vars)...)
    {}

    ObjectDelegate(ObjectDelegate&& other)
        : delegateData(std::move(other.delegateData))
        , executor(std::move(other.executor))
    {}

    ReturnType invoke(Params... params) const override
    {
        return executor.execute<ReturnType>(delegateData.functionPtr, delegateData.objectPtr, std::forward<Params>(params)...);
    }

    virtual bool hasSameObject(const void* object) const override
    {
        return delegateData.objectPtr == object;
    }

private:
    struct FunctionHolder
    {
        FunctionPtr functionPtr;
        const ObjectType* objectPtr;

        FunctionHolder(const ObjectType* object, const FunctionPtr& functionPtr)
            : objectPtr(object)
            , functionPtr(functionPtr)
        {}

        FunctionHolder(FunctionHolder&& other)
            : objectPtr(std::move(objectPtr))
            , functionPtr(std::move(other.functionPtr))
        {
            other.objectPtr = nullptr;
            other.functionPtr = nullptr;
        }

        bool operator==(const FunctionHolder& otherType) const
        {
            return objectPtr == otherType.objectPtr && functionPtr == otherType.functionPtr;
        }
    };

    FunctionHolder delegateData;
    FunctionExecutor<Variables...> executor;
};

template <typename FuncSignature,typename... Variables>
class StaticDelegate;

template <typename ReturnType, typename... Params, typename... Variables>
class StaticDelegate<ReturnType(Params...),Variables...> final : public IDelegate<ReturnType, Params...>
{
public:
    using FunctionPtr = Function<ReturnType, Params..., Variables...>;
    using ThisType = StaticDelegate<ReturnType, Params..., Variables...>;

    explicit StaticDelegate(const FunctionPtr& functionPtr, Variables ...vars)
        : fPtr(functionPtr)
        , executor(vars...)
    {}
    
    StaticDelegate(StaticDelegate&& other)
        : fPtr(std::move(other.fPtr))
        , executor(std::move(other.executor))
    {
        other.fPtr = nullptr;
    }
private:

    ReturnType invoke(Params... params) const override
    {
        return executor.execute<ReturnType>(fPtr, params...);
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

    explicit LambdaDelegate(const FunctionPtr& functionPtr, Variables ...vars)
        : fPtr(functionPtr)
        , executor(vars...)
    {}

    LambdaDelegate(LambdaDelegate&& other)
        : fPtr(std::move(other.fPtr))
        , executor(std::move(other.executor))
    {
        other.fPtr = nullptr;
    }
private:

    ReturnType invoke(Params... params) const override
    {
        return executor.execute<ReturnType>(fPtr, params...);
    }

private:

    FunctionPtr fPtr;
    FunctionExecutor<Variables...> executor;
};

struct DelegateHandle
{
    int32 value = -1;

    bool isValid() const
    {
        return value >= 0;
    }
};

template <typename... Params>
class MultiCastDelegateBase
{
public:
    using DelegateInterface = IDelegate<void, Params...>;
    // Using this function signature to deduce return and params type to individual delegate
    typedef void FuncType(Params...);
protected:
    std::list<SharedPtr<DelegateInterface>> boundStaticDelegates;
    std::list<SharedPtr<DelegateInterface>> boundLambdaDelegates;
	std::list<SharedPtr<DelegateInterface>> boundObjectDelegates;

    std::map<int32, SharedPtr<DelegateInterface>> allDelegates;


    template<typename ObjectType, typename... Variables>
    using ObjDelegateType = ObjectDelegate<false, ObjectType, FuncType, Variables...>;
    template<typename ObjectType, typename... Variables>
    using ConstObjDelegateType = ObjectDelegate<true, ObjectType, FuncType, Variables...>;
    template<typename... Variables>
    using StaticDelegateType = StaticDelegate<FuncType, Variables...>;
    template<typename... Variables>
    using LambdaDelegateType = LambdaDelegate<FuncType, Variables...>;

    int32 getNextId() const
    {
        int32 index = -1;
        for (std::pair<int32, SharedPtr<DelegateInterface>> indexedDelegate : allDelegates)
        {
            index++;
            if (indexedDelegate.first != index) // Map always sorts, so if an index is not equal when equally incrementing it is not available.
            {
                return index;
            }
        }
        return index + 1;
    }

public:
    template<typename ObjectType, typename... Variables>
    std::enable_if_t<!std::is_const_v<ObjectType>, DelegateHandle> bindObject(ObjectType* object
        , const typename ObjDelegateType<ObjectType, Variables...>::FunctionPtr& bindingFunction,Variables ...vars)
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(new ObjDelegateType<ObjectType, Variables...>(object, bindingFunction,vars...));
        boundObjectDelegates.push_back(ptr);
        DelegateHandle handle;
        handle.value = getNextId();
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template<typename ObjectType, typename... Variables>
    DelegateHandle bindObject(const ObjectType* object, const typename ConstObjDelegateType<ObjectType, Variables...>::FunctionPtr& bindingFunction, Variables ...vars)
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(new ConstObjDelegateType<ObjectType, Variables...>(object, bindingFunction, vars...));
        boundObjectDelegates.push_back(ptr);
        DelegateHandle handle;
        handle.value = getNextId();
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template<typename... Variables>
    DelegateHandle bindStatic(const typename StaticDelegateType<Variables...>::FunctionPtr& bindingFunction, Variables ...vars)
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(new StaticDelegateType<Variables...>(bindingFunction, vars...));
        boundStaticDelegates.push_back(ptr);
        DelegateHandle handle;
        handle.value = getNextId();
        allDelegates[handle.value] = ptr;
        return handle;
    }

    template<typename... Variables>
    DelegateHandle bindLambda(const typename LambdaDelegateType<Variables...>::FunctionPtr& lambda, Variables ...vars)
    {
        SharedPtr<DelegateInterface> ptr = SharedPtr<DelegateInterface>(new LambdaDelegateType<Variables...>(lambda, vars...));
        boundLambdaDelegates.push_back(ptr);
        DelegateHandle handle;
        handle.value = getNextId();
        allDelegates[handle.value] = ptr;
        return handle;
    }

    void unbindLambda(const DelegateHandle& handle)
    {
        if (handle.isValid())
        {
            auto itr = allDelegates.find(handle.value);
            if (itr != allDelegates.end())
            {
                boundLambdaDelegates.remove(itr->second);
                allDelegates.erase(itr);
            }
        }
    }

    void unbindStatic(const DelegateHandle& handle)
    {
        if (handle.isValid())
        {
            auto itr = allDelegates.find(handle.value);
            if (itr != allDelegates.end())
            {
                boundStaticDelegates.remove(itr->second);
                allDelegates.erase(itr);
            }
        }
    }

    void unbindObject(const DelegateHandle& handle)
    {
        if (handle.isValid())
        {
            auto itr = allDelegates.find(handle.value);
            if (itr != allDelegates.end())
            {
                boundObjectDelegates.remove(itr->second);
                allDelegates.erase(itr);
            }
        }
    }

    template<typename ObjectType>
    void unbindAll(const ObjectType* object)
    {
        boundObjectDelegates.remove_if([object](const SharedPtr<DelegateInterface>& delegateInterface)
            {
                // No need to check null as all object delegates implements this 
                return delegateInterface->hasSameObject(object);
            });

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

    bool isBound() const
    {
        return boundObjectDelegates.size() > 0 || boundStaticDelegates.size() > 0 || boundLambdaDelegates.size() > 0;
    }

    void clear();
};

/* Does not handle life time of object delegates needs to be externally handled */
template <typename... Params>
class Delegate : public MultiCastDelegateBase<Params...>
{
private:
    using MultiCastDelegateBase<Params...>::boundObjectDelegates;
    using MultiCastDelegateBase<Params...>::boundLambdaDelegates;
    using MultiCastDelegateBase<Params...>::boundStaticDelegates;
    using MultiCastDelegateBase<Params...>::DelegateInterface;
public:
    using MultiCastDelegateBase<Params...>::clear;
    ~Delegate();

    void invoke(Params... params) const;
};

using SimpleDelegate = Delegate<>;

template <typename... Params>
Delegate<Params...>::~Delegate()
{
    clear();
}

template <typename... Params>
void Delegate<Params...>::invoke(Params... params) const
{
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundObjectDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundStaticDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundLambdaDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
}

/* Does not handle life time of object delegates needs to be externally handled */
template <typename OwnerType,typename... Params>
class Event : public MultiCastDelegateBase<Params...>
{
private:
    using MultiCastDelegateBase<Params...>::boundObjectDelegates;
    using MultiCastDelegateBase<Params...>::boundLambdaDelegates;
    using MultiCastDelegateBase<Params...>::boundStaticDelegates;
    using MultiCastDelegateBase<Params...>::DelegateInterface;

    friend OwnerType;

    void invoke(Params... params) const;
public:
    using MultiCastDelegateBase<Params...>::clear;
    ~Event();

};

template <typename OwnerType, typename... Params>
Event<OwnerType, Params...>::~Event()
{
    clear();
}

template <typename OwnerType, typename... Params>
void Event<OwnerType, Params...>::invoke(Params... params) const
{
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundObjectDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundStaticDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
    for (const SharedPtr<DelegateInterface>& delegateInterface : boundLambdaDelegates)
    {
        delegateInterface->invoke(std::forward<Params>(params)...);
    }
}

template <typename... Params>
void MultiCastDelegateBase<Params...>::clear()
{
    boundStaticDelegates.clear();
    boundObjectDelegates.clear();
    boundLambdaDelegates.clear();
    allDelegates.clear();
}


