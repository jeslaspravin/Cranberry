#pragma once

#include "Reflections/Functions.h"
#include "Types/TypesInfo.h"
#include "Logger/Logger.h"


class BaseFunctionWrapper
{
private:
    const ReflectTypeInfo* returnTypeInfo;
    std::vector<const ReflectTypeInfo*> argsTypeInfo;
protected:
    virtual const void* functionAccessor() const = 0;
public:
    BaseFunctionWrapper(const ReflectTypeInfo* retType, std::vector<const ReflectTypeInfo*>&& argsType)
        : returnTypeInfo(retType)
        , argsTypeInfo(std::move(argsType))
    {}
    BaseFunctionWrapper(const ReflectTypeInfo* retType, const std::vector<const ReflectTypeInfo*>& argsType)
        : returnTypeInfo(retType)
        , argsTypeInfo(argsType)
    {}

    virtual ~BaseFunctionWrapper() = default;

    const ReflectTypeInfo* getReturnTypeInfo() const
    {
        return returnTypeInfo;
    }

    // Including all CV-Ref qualifiers
    template <typename CheckType>
    FORCE_INLINE bool isSameReturnType() const
    {
        return returnTypeInfo == typeInfoFrom<CheckType>();
    }

    template <typename... Args>
    FORCE_INLINE bool isSameArgsType() const
    {
        std::vector<const ReflectTypeInfo*> checkArgsType = typeInfoListFrom<Args...>();
        if (checkArgsType.size() == argsTypeInfo.size())
        {
            for (uint32 i = 0; i < checkArgsType.size(); ++i)
            {
                if (argsTypeInfo[i] != checkArgsType[i])
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};


class MemberFunctionWrapper : public BaseFunctionWrapper
{
private:
    const ReflectTypeInfo* memberOfType;
    // Inner most type from member of type
    const ReflectTypeInfo* memberOfTypeInner;

private:
    FORCE_INLINE static const ReflectTypeInfo* getInnerMostType(const ReflectTypeInfo* typeInfo)
    {
        const ReflectTypeInfo* innerMostType = typeInfo;
        while (innerMostType->innerType)
        {
            innerMostType = innerMostType->innerType;
        }
        return innerMostType;
    }
public:
    MemberFunctionWrapper(const ReflectTypeInfo* outerClassType, const ReflectTypeInfo* retType, std::vector<const ReflectTypeInfo*>&& argsType)
        : BaseFunctionWrapper(retType, std::move(argsType))
        , memberOfType(outerClassType)
        , memberOfTypeInner(getInnerMostType(memberOfType))
    {
    }
    MemberFunctionWrapper(const ReflectTypeInfo* outerClassType, const ReflectTypeInfo* retType, const std::vector<const ReflectTypeInfo*>& argsType)
        : BaseFunctionWrapper(retType, argsType)
        , memberOfType(outerClassType)
        , memberOfTypeInner(getInnerMostType(memberOfType))
    {}

    template <typename ObjectType, typename ReturnType, typename... Args>
    bool invoke(ObjectType&& object, ReturnType& returnVal, Args&&... params) const
    {
        if (!(isSameReturnType<ReturnType>() && isSameArgsType<Args...>()))
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given return values and arguments", __func__);
            return false;
        }
        // Since type ID of the innermost type must be same with object type's inner most type
        const ReflectTypeInfo* objectInnerMostType = getInnerMostType(typeInfoFrom<ObjectType>());
        if (objectInnerMostType->typeID != memberOfTypeInner->typeID)
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given object type", __func__);
            return false;
        }

        if (BIT_SET(memberOfType->qualifiers, EReflectTypeQualifiers::Constant))
        {
            ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...>* functionPtr
                = (ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...>*)(functionAccessor());

            returnVal = (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
        }
        else
        {
            // Cannot invoke on const type
            if CONST_EXPR(std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
            {
                return false;
            }
            else
            {
                ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...>* functionPtr
                    = (ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...>*)(functionAccessor());
                returnVal = (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
            }
        }
        return true;
    }

    template <typename ObjectType, typename... Args>
    bool invokeVoid(ObjectType&& object, Args&&... params) const
    {
        if (!(isSameReturnType<void>() && isSameArgsType<Args...>()))
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given return values and arguments", __func__);
            return false;
        }
        // Since type ID of the innermost type must be same with object type's inner most type
        const ReflectTypeInfo* objectInnerMostType = getInnerMostType(typeInfoFrom<ObjectType>());
        if (objectInnerMostType->typeID != memberOfTypeInner->typeID)
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given object type", __func__);
            return false;
        }

        if (BIT_SET(memberOfType->qualifiers, EReflectTypeQualifiers::Constant))
        {
            const ClassFunction<true, CleanType<ObjectType>, void, Args...>* functionPtr
                = (const ClassFunction<true, CleanType<ObjectType>, void, Args...>*)(functionAccessor());

            (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
        }
        else
        {
            // Cannot invoke on const type
            if CONST_EXPR(std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
            {
                return false;
            }
            else
            {
                const ClassFunction<false, CleanType<ObjectType>, void, Args...>* functionPtr
                    = (const ClassFunction<false, CleanType<ObjectType>, void, Args...>*)(functionAccessor());

                (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
            }
        }
        return true;
    }
};

class GlobalFunctionWrapper : public BaseFunctionWrapper
{
public:
    GlobalFunctionWrapper(const ReflectTypeInfo* retType, std::vector<const ReflectTypeInfo*>&& argsType)
        : BaseFunctionWrapper(retType, std::move(argsType))
    {}
    GlobalFunctionWrapper(bool modifiesClass, const ReflectTypeInfo* retType, const std::vector<const ReflectTypeInfo*>& argsType)
        : BaseFunctionWrapper(retType, argsType)
    {}

    template <typename ReturnType, typename... Args>
    bool invoke(ReturnType& returnVal, Args&&... params) const
    {
        if (!(isSameReturnType<ReturnType>() && isSameArgsType<Args...>()))
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given return values and arguments", __func__);
            return false;
        }

        const Function<ReturnType, Args...>* functionPtr
            = (const Function<ReturnType, Args...>*)(functionAccessor());

        returnVal = (*functionPtr)(std::forward<Args>(params)...);

        return true;
    }
    template <typename... Args>
    bool invokeVoid(Args&&... params) const
    {
        if (!(isSameReturnType<void>() && isSameArgsType<Args...>()))
        {
            Logger::error("MemberFunctionWrapper", "%s() : Cannot call this function with given return values and arguments", __func__);
            return false;
        }

        const Function<void, Args...>* functionPtr
            = (const Function<void, Args...>*)(functionAccessor());

        (*functionPtr)(std::forward<Args>(params)...);

        return true;
    }
};

// Templated Implementations

template <typename ObjectType, typename ReturnType, typename... Args>
class MemberFunctionWrapperImpl : public MemberFunctionWrapper
{
private:
    using MemberFunction = ClassFunction<std::is_const_v<ObjectType>, std::remove_const_t<ObjectType>, ReturnType, Args...>;

    MemberFunction memberFunc;
protected:
    const void* functionAccessor() const override
    {
        return &memberFunc;
    }
public:
    MemberFunctionWrapperImpl(MemberFunction::ClassDelegate funcPtr)
        : MemberFunctionWrapper(typeInfoFrom<ObjectType>(), typeInfoFrom<ReturnType>(), std::move(typeInfoListFrom<Args...>()))
        , memberFunc(funcPtr)
    {}
};

template <typename ReturnType, typename... Args>
class GlobalFunctionWrapperImpl : public GlobalFunctionWrapper
{
private:
    using GlobalFunction = Function<ReturnType, Args...>;

    GlobalFunction func;
protected:
    const void* functionAccessor() const override
    {
        return &func;
    }
public:
    GlobalFunctionWrapperImpl(GlobalFunction::StaticDelegate funcPtr)
        : GlobalFunctionWrapper(typeInfoFrom<ReturnType>(), std::move(typeInfoListFrom<Args...>()))
        , func(funcPtr)
    {}
};