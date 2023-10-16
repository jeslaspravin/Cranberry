/*!
 * \file FunctionTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Logger/Logger.h"
#include "Reflections/Functions.h"
#include "Types/TypesInfo.h"
#include "Reflections/FunctionParamsStack.h"

class BaseFunctionWrapper
{
private:
    const ReflectTypeInfo *returnTypeInfo;
    std::vector<const ReflectTypeInfo *> argsTypeInfo;

protected:
    virtual const void *functionAccessor() const = 0;

public:
    BaseFunctionWrapper(const ReflectTypeInfo *retType, std::vector<const ReflectTypeInfo *> &&argsType)
        : returnTypeInfo(retType)
        , argsTypeInfo(std::move(argsType))
    {}
    BaseFunctionWrapper(const ReflectTypeInfo *retType, const std::vector<const ReflectTypeInfo *> &argsType)
        : returnTypeInfo(retType)
        , argsTypeInfo(argsType)
    {}

    virtual ~BaseFunctionWrapper() = default;

    const ReflectTypeInfo *getReturnTypeInfo() const { return returnTypeInfo; }

    // Including all CV-Ref qualifiers
    template <typename CheckType>
    FORCE_INLINE bool isSameReturnType() const
    {
        return returnTypeInfo == typeInfoFrom<CheckType>();
    }

    template <typename... Args>
    FORCE_INLINE bool isSameArgsType() const
    {
        std::vector<const ReflectTypeInfo *> checkArgsType = typeInfoListFrom<Args...>();
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
    const ReflectTypeInfo *memberOfType;
    // Inner most type from member of type
    const ReflectTypeInfo *memberOfTypeInner;

private:
    FORCE_INLINE static const ReflectTypeInfo *getInnerMostType(const ReflectTypeInfo *typeInfo)
    {
        const ReflectTypeInfo *innerMostType = typeInfo;
        while (innerMostType->innerType)
        {
            innerMostType = innerMostType->innerType;
        }
        return innerMostType;
    }

public:
    MemberFunctionWrapper(
        const ReflectTypeInfo *outerClassType, const ReflectTypeInfo *retType, std::vector<const ReflectTypeInfo *> &&argsType
    )
        : BaseFunctionWrapper(retType, std::move(argsType))
        , memberOfType(outerClassType)
        , memberOfTypeInner(getInnerMostType(memberOfType))
    {}
    MemberFunctionWrapper(
        const ReflectTypeInfo *outerClassType, const ReflectTypeInfo *retType, const std::vector<const ReflectTypeInfo *> &argsType
    )
        : BaseFunctionWrapper(retType, argsType)
        , memberOfType(outerClassType)
        , memberOfTypeInner(getInnerMostType(memberOfType))
    {}

    virtual bool invokeTypeless(void *object, void *returnVal, void *argsStack, SizeT argsByteSize) const = 0;

    template <typename ObjectType, typename ReturnType, typename... Args>
    bool invoke(ObjectType &&object, ReturnType &returnVal, Args... params) const
    {
        if (!(isSameReturnType<ReturnType>() && isSameArgsType<Args...>()))
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given return values and arguments");
            return false;
        }
        // Since type ID of the innermost type must be same with object type's inner most type
        const ReflectTypeInfo *objectInnerMostType = getInnerMostType(typeInfoFrom<ObjectType>());
        if (objectInnerMostType->typeID != memberOfTypeInner->typeID)
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given object type");
            return false;
        }

        if (BIT_SET(memberOfType->qualifiers, EReflectTypeQualifiers::Constant))
        {
            ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...> *functionPtr
                = (ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...> *)(functionAccessor());

            returnVal = (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
        }
        else
        {
            // Cannot invoke on const type
            if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
            {
                return false;
            }
            else
            {
                ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...> *functionPtr
                    = (ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...> *)(functionAccessor());
                returnVal = (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
            }
        }
        return true;
    }

    template <typename ObjectType, typename... Args>
    bool invokeVoid(ObjectType &&object, Args... params) const
    {
        if (!(isSameReturnType<void>() && isSameArgsType<Args...>()))
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given return values and arguments");
            return false;
        }
        // Since type ID of the innermost type must be same with object type's inner most type
        const ReflectTypeInfo *objectInnerMostType = getInnerMostType(typeInfoFrom<ObjectType>());
        if (objectInnerMostType->typeID != memberOfTypeInner->typeID)
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given object type");
            return false;
        }

        if (BIT_SET(memberOfType->qualifiers, EReflectTypeQualifiers::Constant))
        {
            const ClassFunction<true, CleanType<ObjectType>, void, Args...> *functionPtr
                = (const ClassFunction<true, CleanType<ObjectType>, void, Args...> *)(functionAccessor());

            (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
        }
        else
        {
            // Cannot invoke on const type
            if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
            {
                return false;
            }
            else
            {
                const ClassFunction<false, CleanType<ObjectType>, void, Args...> *functionPtr
                    = (const ClassFunction<false, CleanType<ObjectType>, void, Args...> *)(functionAccessor());

                (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
            }
        }
        return true;
    }

    template <typename ObjectType, typename ReturnType, typename... Args>
    ReturnType invokeUnsafe(ObjectType &&object, Args... params) const
    {
        if (BIT_SET(memberOfType->qualifiers, EReflectTypeQualifiers::Constant))
        {
            ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...> *functionPtr
                = (ClassFunction<true, CleanType<ObjectType>, ReturnType, Args...> *)(functionAccessor());

            return (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
        }
        else
        {
            ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...> *functionPtr
                = (ClassFunction<false, CleanType<ObjectType>, ReturnType, Args...> *)(functionAccessor());

            if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
            {
                fatalAssertf(
                    !std::is_const_v<UnderlyingTypeWithConst<ObjectType>>, "Const type cannot invoke non const object member function"
                );
            }
            else
            {
                return (*functionPtr)(std::forward<ObjectType>(object), std::forward<Args>(params)...);
            }
        }
    }
};

class GlobalFunctionWrapper : public BaseFunctionWrapper
{
public:
    GlobalFunctionWrapper(const ReflectTypeInfo *retType, std::vector<const ReflectTypeInfo *> &&argsType)
        : BaseFunctionWrapper(retType, std::move(argsType))
    {}
    GlobalFunctionWrapper(bool, const ReflectTypeInfo *retType, const std::vector<const ReflectTypeInfo *> &argsType)
        : BaseFunctionWrapper(retType, argsType)
    {}

    virtual bool invokeTypeless(void *returnVal, void *argsStack, SizeT argsByteSize) const = 0;

    template <typename ReturnType, typename... Args>
    bool invoke(ReturnType &returnVal, Args... params) const
    {
        if (!(isSameReturnType<ReturnType>() && isSameArgsType<Args...>()))
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given return values and arguments");
            return false;
        }

        const Function<ReturnType, Args...> *functionPtr = (const Function<ReturnType, Args...> *)(functionAccessor());

        returnVal = (*functionPtr)(std::forward<Args>(params)...);

        return true;
    }
    template <typename... Args>
    bool invokeVoid(Args... params) const
    {
        if (!(isSameReturnType<void>() && isSameArgsType<Args...>()))
        {
            LOG_ERROR("MemberFunctionWrapper", "Cannot call this function with given return values and arguments");
            return false;
        }

        const Function<void, Args...> *functionPtr = (const Function<void, Args...> *)(functionAccessor());

        (*functionPtr)(std::forward<Args>(params)...);

        return true;
    }

    template <typename ReturnType, typename... Args>
    ReturnType invokeUnsafe(Args... params) const
    {
        const Function<ReturnType, Args...> *functionPtr = (const Function<ReturnType, Args...> *)(functionAccessor());

        return (*functionPtr)(std::forward<Args>(params)...);
    }
};

//////////////////////////////////////////////////////////////////////////
// Templated Implementations
//////////////////////////////////////////////////////////////////////////

template <typename ObjectType, typename ReturnType, typename... Args>
class MemberFunctionWrapperImpl : public MemberFunctionWrapper
{
private:
    using MemberFunction = ClassFunction<std::is_const_v<ObjectType>, std::remove_const_t<ObjectType>, ReturnType, Args...>;

    MemberFunction memberFunc;

protected:
    const void *functionAccessor() const override { return &memberFunc; }

public:
    MemberFunctionWrapperImpl(typename MemberFunction::ClassDelegate funcPtr)
        : MemberFunctionWrapper(typeInfoFrom<ObjectType>(), typeInfoFrom<ReturnType>(), std::move(typeInfoListFrom<Args...>()))
        , memberFunc(funcPtr)
    {}
    /* MemberFunctionWrapper overrides */
    bool invokeTypeless(void *object, void *returnVal, void *argsStack, SizeT argsByteSize) const override
    {
        if (FunctionParamsStack::canInvokeWithStack<Args...>(argsByteSize))
        {
            ObjectType *outerObject = (ObjectType *)(object);
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                FunctionParamsStack::invoke(memberFunc, outerObject, (uint8 *)argsStack, argsByteSize);
            }
            else
            {
                ReturnType *retValPtr = (ReturnType *)returnVal;
                *retValPtr = FunctionParamsStack::invoke(memberFunc, outerObject, (uint8 *)argsStack, argsByteSize);
            }
            return true;
        }
        return false;
    }
    /* Override ends */
};

template <typename ReturnType, typename... Args>
class GlobalFunctionWrapperImpl : public GlobalFunctionWrapper
{
private:
    using GlobalFunction = Function<ReturnType, Args...>;

    GlobalFunction func;

protected:
    const void *functionAccessor() const override { return &func; }

public:
    GlobalFunctionWrapperImpl(typename GlobalFunction::StaticDelegate funcPtr)
        : GlobalFunctionWrapper(typeInfoFrom<ReturnType>(), std::move(typeInfoListFrom<Args...>()))
        , func(funcPtr)
    {}

    /* GlobalFunctionWrapper overrides */
    bool invokeTypeless(void *returnVal, void *argsStack, SizeT argsByteSize) const override
    {
        if (FunctionParamsStack::canInvokeWithStack<Args...>(argsByteSize))
        {
            if constexpr (std::is_same_v<ReturnType, void>)
            {
                FunctionParamsStack::invoke(func, (uint8 *)argsStack, argsByteSize);
            }
            else
            {
                ReturnType *retValPtr = (ReturnType *)returnVal;
                *retValPtr = FunctionParamsStack::invoke(func, (uint8 *)argsStack, argsByteSize);
            }
            return true;
        }
        return false;
    }
    /* Override ends */
};