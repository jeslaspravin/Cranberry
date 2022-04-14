/*!
 * \file FunctionParamsStack.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Math.h"
#include "Reflections/Functions.h"
#include "Types/CoreTypes.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Templates/TypeList.h"

namespace FunctionParamsStack
{
// For reference value alone remove reference and make it as pointer as reference param is just its
// pointer being passed around
template <typename Type>
using ParamTypeToStackType
    = std::conditional_t<std::is_reference<Type>::value, std::remove_reference_t<Type> *, Type>;

template <typename TList>
struct CalculateParamsAlignedLayout;
template <typename Type>
struct CalculateParamsAlignedLayout<TypeList<Type, NullType>>
{
    using T = ParamTypeToStackType<Type>;
    CONST_EXPR static const SizeT offset = 0;
    CONST_EXPR static const SizeT value = sizeof(T);
};
template <typename Type, typename TList>
struct CalculateParamsAlignedLayout<TypeList<Type, TList>>
{
    using T = ParamTypeToStackType<Type>;
    CONST_EXPR static const SizeT offset
        = Math::alignByUnsafe(CalculateParamsAlignedLayout<TList>::value, alignof(T));
    CONST_EXPR static const SizeT value = offset + sizeof(T);
};
// Calculates the alignment of struct if types in list are member of the struct
template <typename TList>
struct CalculateMaxAlignment;
template <typename Type>
struct CalculateMaxAlignment<TypeList<Type, NullType>>
{
    using T = ParamTypeToStackType<Type>;
    CONST_EXPR static const SizeT value = alignof(T);
};
template <typename Type, typename TList>
struct CalculateMaxAlignment<TypeList<Type, TList>>
{
    using T = ParamTypeToStackType<Type>;
    CONST_EXPR static const SizeT NextMax = CalculateMaxAlignment<TList>::value;
    CONST_EXPR static const SizeT value = (alignof(T) > NextMax) ? alignof(T) : NextMax;
};

template <typename Type, typename UserType>
struct StoreTypeOffsets
{
    using T = ParamTypeToStackType<Type>;

    FORCE_INLINE void operator()(UserType *data) const
    {
        SizeT offset = Math::alignByUnsafe(data->size, alignof(T));
        data->offsetsPtr[data->idx] = offset;
        data->size = offset + sizeof(T);
        data->idx++;
    }
};

// Given a list of types returns offsets of each types if each type data is placed in continuous aligned
// memory stack, First type will be in lowest memory location and last type in highest
template <TypeListType TList>
std::vector<SizeT> paramsSizeAndOffsets(SizeT &outSize)
{
    std::vector<SizeT> offsets(TL::Length<TList>::value);

    struct ParamsSizeOffsetData
    {
        SizeT *offsetsPtr;
        SizeT size = 0;
        uint32_t idx = 0;
    } sizeAndOffsets;
    sizeAndOffsets.offsetsPtr = offsets.data();

    TL::DoForEach<TList, StoreTypeOffsets>::call(&sizeAndOffsets);
    outSize = sizeAndOffsets.size;
    return offsets;
}

template <SizeT size, SizeT align = 16>
struct alignas(align) ParamsStackData
{
    uint8 vals[size];
};
template <SizeT align>
struct ParamsStackData<0, align>
{};

template <typename TList>
struct PushPopStackedData
{
public:
    using RTList = typename TL::Reverse<TList>::type;
    CONST_EXPR static const SizeT Len = TL::Length<TList>::value;

    template <typename Type>
    CONST_EXPR static void pushAnArg(uint8 *data, Type arg)
    {
        if CONST_EXPR (std::is_reference<Type>::value)
        {
            *reinterpret_cast<std::remove_reference_t<Type> **>(data) = &arg;
        }
        else if CONST_EXPR (std::is_pointer<Type>::value)
        {
            *reinterpret_cast<std::remove_reference_t<Type> *>(data) = arg;
        }
        else
        {
            Type &dataRef = *reinterpret_cast<Type *>(data);
            if CONST_EXPR (std::is_constructible<Type>::value)
            {
                new (&dataRef) Type(arg);
            }
            else
            {
                dataRef = arg;
            }
        }
    }
    template <typename Arg, SizeT Index>
    CONST_EXPR static void pushToStackedData(uint8 *data, Arg arg, std::index_sequence<Index>)
    {
        pushAnArg<Arg>(data
                           + CalculateParamsAlignedLayout<
                               typename TL::AtIndex<RTList, Len - 1 - Index>::TList>::offset,
            std::forward<Arg>(arg));
    }
    template <typename Arg, typename... Args, SizeT Index, SizeT... Indices>
    CONST_EXPR static void pushToStackedData(
        uint8 *data, Arg arg, Args... args, std::index_sequence<Index, Indices...>)
    {
        pushAnArg<Arg>(data
                           + CalculateParamsAlignedLayout<
                               typename TL::AtIndex<RTList, Len - 1 - Index>::TList>::offset,
            std::forward<Arg>(arg));
        pushToStackedData<Args...>(data, std::forward<Args>(args)..., std::index_sequence<Indices...>{});
    }

    template <SizeT Idx, typename Type = typename TL::AtIndex<TList, Idx>::type>
    NODISCARD static Type &popAnArg(uint8 *data)
    {
        data
            = data
              + CalculateParamsAlignedLayout<typename TL::AtIndex<RTList, Len - 1 - Idx>::TList>::offset;
        if CONST_EXPR (std::is_reference<Type>::value)
        {
            return **reinterpret_cast<std::remove_reference_t<Type> **>(data);
        }
        else if CONST_EXPR (std::is_pointer<Type>::value)
        {
            return *reinterpret_cast<std::remove_reference_t<Type> *>(data);
        }
        else
        {
            return *reinterpret_cast<Type *>(data);
        }
    }
    template <template <typename RetType, typename... Params> typename FuncType, typename RetType,
        typename... Args, SizeT... Indices>
    static RetType invoke(
        const FuncType<RetType, Args...> &func, uint8 *data, SizeT size, std::index_sequence<Indices...>)
    {
        return func(popAnArg<Indices>(data)...);
    }

    template <bool bIsConst, typename ClassType, typename ObjectType, typename RetType, typename... Args,
        SizeT... Indices>
    static RetType invoke(const ClassFunction<bIsConst, ClassType, RetType, Args...> &func,
        ObjectType &&object, uint8 *data, SizeT size, std::index_sequence<Indices...>)
    {
        return func(object, popAnArg<Indices>(data)...);
    }
};

template <typename... Args>
NODISCARD CONST_EXPR auto pushToStackedData(Args... args)
{
    using TypesL = typename TL::CreateFrom<Args...>::type;
    ParamsStackData<CalculateParamsAlignedLayout<typename TL::Reverse<TypesL>::type>::value,
        CalculateMaxAlignment<TypesL>::value>
        ds;
    PushPopStackedData<TypesL>::template pushToStackedData<Args...>(
        ds.vals, std::forward<Args>(args)..., std::make_index_sequence<sizeof...(Args)>{});
    return ds;
}
CONST_EXPR ParamsStackData<0> pushToStackedData() { return {}; }

// For both lambda and static global functions
template <template <typename RetType, typename... Params> typename FuncType, typename RetType,
    typename... Args>
RetType invoke(const FuncType<RetType, Args...> &func, uint8 *data, SizeT size)
{
    using TypesL = typename TL::CreateFrom<Args...>::type;
    // Passed in size must be greater than or  equal to required size
    debugAssert(CalculateParamsAlignedLayout<TypesL>::value <= size);
    return PushPopStackedData<TypesL>::template invoke<FuncType, RetType, Args...>(
        func, data, size, std::make_index_sequence<sizeof...(Args)>{});
}
template <template <typename RetType, typename... Params> typename FuncType, typename RetType>
RetType invoke(const FuncType<RetType> &func, uint8 *data, SizeT size)
{
    return func();
}

// For object functions
template <bool bIsConst, typename ClassType, typename ObjectType, typename RetType, typename... Args>
RetType invoke(const ClassFunction<bIsConst, ClassType, RetType, Args...> &func, ObjectType &&object,
    uint8 *data, SizeT size)
{
    using TypesL = typename TL::CreateFrom<Args...>::type;
    // Passed in size must be greater than or  equal to required size
    debugAssert(CalculateParamsAlignedLayout<TypesL>::value <= size);
    return PushPopStackedData<TypesL>::template invoke<bIsConst, ClassType, ObjectType, RetType,
        Args...>(
        func, std::forward<ObjectType>(object), data, size, std::make_index_sequence<sizeof...(Args)>{});
}
template <bool bIsConst, typename ClassType, typename ObjectType, typename RetType>
RetType invoke(const ClassFunction<bIsConst, ClassType, RetType> &func, ObjectType &&object, uint8 *data,
    SizeT size)
{
    return func(std::forward<ObjectType>(object));
}
} // namespace FunctionParamsStack