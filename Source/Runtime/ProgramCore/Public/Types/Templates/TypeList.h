/*!
 * \file TypeList.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

/*
 * Creates a typelist which can be used later to convert to variadic type and querying and modification
 * of types are supported as well
 *
 * Usages
 *
 *    fmt::print("Length {}\nWith TestTL :", TL::Length<TestTL>::value);
 *    printTypeInfo(typeid(TL::MostDerived<TestTL, TestTL::Type>::type));
 *    TL::DoForEach<TL::DerivedToFront<TestTL>::type, PrintTypeInfo>::call();
 *    fmt::print("\nLength {}\nWith TestTL2 :", TL::Length<TestTL>::value);
 *    TL::DoForEach<TL::DerivedToFront<TestTL2>::type, PrintTypeInfo>::call();
 *    printTypeInfo(typeid(TL::MostDerived<TestTL2, TestTL2::Type>::type));
 *    fmt::print("\nIndex of Widget {}", TL::TypeIndex<TestTL, Widget>::value);
 *    fmt::print("\nAt index 1 {}", typeid(TL::AtIndex<TestTL, 1>::type).name());
 *    fmt::print("\nLength Dup {}", TL::Length<TestTLDup>::value);
 *    TL::DoForEach<TL::ApplyAll<TestTLDup, TL::CreateFrom>::type::type, PrintTypeInfo>::call();
 *    fmt::print("\nReverse Dup Length {}", TL::Length<TestTLDup>::value);
 *    TL::DoForEach<TL::Reverse<TestTLDup>::type, PrintTypeInfo>::call();
 *    fmt::print("\nLength RemovedDup {}", TL::Length<TL::RemoveDuplicates<TestTLDup>::type>::value);
 *    TL::DoForEach<TL::RemoveDuplicates<TestTLDup>::type, PrintTypeInfo>::call();
 *
 *
 */

#include "Types/Templates/TemplateTypes.h"

#include <type_traits>

template <typename ThisType, typename NextType>
struct TypeList
{
    using Type = ThisType;
    using Next = NextType;
};

template <typename T>
struct IsTypeListType : std::false_type
{};

template <typename Type, typename Next>
struct IsTypeListType<TypeList<Type, Next>>
{
    // This is typelist, now make sure the chain is typelist as well
    CONST_EXPR static const bool value = std::disjunction_v<IsTypeListType<Next>, std::is_same<Next, NullType>>;
};

template <typename T>
concept TypeListType = IsTypeListType<T>::value;

namespace TL
{
// Appends a type to end of this type list
template <typename TList, typename AppendType>
struct Append;

template <typename AppendType>
struct Append<NullType, AppendType>
{
    using type = TypeList<AppendType, NullType>;
};
template <typename ThisType, typename NextType, typename AppendType>
struct Append<TypeList<ThisType, NextType>, AppendType>
{
    using type = TypeList<ThisType, typename Append<NextType, AppendType>::type>;
};

// Replaces All matching type
template <typename TList, typename FromType, typename ToType>
struct ReplaceAll;

template <typename FromType, typename ToType>
struct ReplaceAll<NullType, FromType, ToType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType, typename ToType>
struct ReplaceAll<TypeList<ThisType, NextType>, ThisType, ToType>
{
    using type = TypeList<ToType, typename ReplaceAll<NextType, ThisType, ToType>::type>;
};
template <typename ThisType, typename NextType, typename FromType, typename ToType>
struct ReplaceAll<TypeList<ThisType, NextType>, FromType, ToType>
{
    using type = TypeList<ThisType, typename ReplaceAll<NextType, FromType, ToType>::type>;
};

// Replaces first matching type
template <typename TList, typename FromType, typename ToType>
struct Replace;

template <typename FromType, typename ToType>
struct Replace<NullType, FromType, ToType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType, typename ToType>
struct Replace<TypeList<ThisType, NextType>, ThisType, ToType>
{
    using type = TypeList<ToType, NextType>;
};
template <typename ThisType, typename NextType, typename FromType, typename ToType>
struct Replace<TypeList<ThisType, NextType>, FromType, ToType>
{
    using type = TypeList<ThisType, typename Replace<NextType, FromType, ToType>::type>;
};

// Erases all matching type
template <typename TList, typename EraseType>
struct EraseAll;

template <typename EraseType>
struct EraseAll<NullType, EraseType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType>
struct EraseAll<TypeList<ThisType, NextType>, ThisType>
{
    using type = typename EraseAll<NextType, ThisType>::type;
};
template <typename ThisType, typename NextType, typename EraseType>
struct EraseAll<TypeList<ThisType, NextType>, EraseType>
{
    using type = TypeList<ThisType, typename EraseAll<NextType, EraseType>::type>;
};

// Erases first matching type
template <typename TList, typename EraseType>
struct Erase;

template <typename EraseType>
struct Erase<NullType, EraseType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType>
struct Erase<TypeList<ThisType, NextType>, ThisType>
{
    using type = NextType;
};
template <typename ThisType, typename NextType, typename EraseType>
struct Erase<TypeList<ThisType, NextType>, EraseType>
{
    using type = TypeList<ThisType, typename Erase<NextType, EraseType>::type>;
};

// Removes all duplicates and preserves order
template <typename TList>
struct RemoveDuplicates;

template <>
struct RemoveDuplicates<NullType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType>
struct RemoveDuplicates<TypeList<ThisType, NextType>>
{
    using type = TypeList<ThisType, typename RemoveDuplicates<typename EraseAll<NextType, ThisType>::type>::type>;
};

// Finds length of type list
template <typename TList>
struct Length;

template <>
struct Length<NullType>
{
    CONST_EXPR static const int value = 0;
};
template <typename ThisType, typename NextType>
struct Length<TypeList<ThisType, NextType>>
{
    CONST_EXPR static const int value = 1 + Length<NextType>::value;
};

// Finds type at an index
template <typename TList, int Index>
struct AtIndex;

template <typename ThisType, typename NextType>
struct AtIndex<TypeList<ThisType, NextType>, 0>
{
    using type = ThisType;
    using Next = NextType;
    using TList = TypeList<type, Next>;
};
template <typename ThisType, typename NextType, int Index>
struct AtIndex<TypeList<ThisType, NextType>, Index>
{
    using AtIndexTList = AtIndex<NextType, Index - 1>;
    using type = typename AtIndexTList::type;
    using Next = typename AtIndexTList::Next;
    using TList = TypeList<type, Next>;
};

// Finds the first index of the type in typelist
template <typename TList, typename FindType>
struct TypeIndex;

template <typename FindType>
struct TypeIndex<NullType, FindType>
{
    CONST_EXPR static const int value = -1;
};
template <typename NextType, typename FindType>
struct TypeIndex<TypeList<NullType, NextType>, FindType>
{
    CONST_EXPR static const int value = -1;
};
template <typename ThisType, typename NextType>
struct TypeIndex<TypeList<ThisType, NextType>, ThisType>
{
    CONST_EXPR static const int value = 0;
};
template <typename ThisType, typename NextType, typename FindType>
struct TypeIndex<TypeList<ThisType, NextType>, FindType>
{
    CONST_EXPR static const int foundAt = TypeIndex<NextType, FindType>::value;
    CONST_EXPR static const int value = (foundAt == -1) ? -1 : 1 + foundAt;
};

template <typename TList, typename FindType>
struct Contains
{
    CONST_EXPR static const bool value = (TypeIndex<TList, FindType>::value != -1);
};

// Reverses the Typelist,
template <typename TList>
struct Reverse;

template <typename ThisType>
struct Reverse<TypeList<ThisType, NullType>>
{
    using type = TypeList<ThisType, NullType>;
};
template <typename ThisType, typename NextType>
struct Reverse<TypeList<ThisType, NextType>>
{
    using type = typename Append<typename Reverse<NextType>::type, ThisType>::type;
};

// Find the deepest child from the head type
template <typename TList, typename BaseType>
struct MostDerived;

template <typename BaseType>
struct MostDerived<NullType, BaseType>
{
    using type = BaseType;
};
template <typename ThisType, typename NextType, typename BaseType>
struct MostDerived<TypeList<ThisType, NextType>, BaseType>
{
    using MostDerivedType = typename MostDerived<NextType, BaseType>::type;
    using type = std::conditional_t<std::is_convertible_v<ThisType, MostDerivedType>, ThisType, MostDerivedType>;
};

// Deepest inherited type will be brought to index 0 and highest parent will be brough to Length - 1
template <typename TList>
struct DerivedToFront;

template <>
struct DerivedToFront<NullType>
{
    using type = NullType;
};
template <typename ThisType, typename NextType>
struct DerivedToFront<TypeList<ThisType, NextType>>
{
    using MostDerivedType = typename MostDerived<NextType, ThisType>::type;
    using NewNextType = typename ReplaceAll<NextType, MostDerivedType, ThisType>::type;

    using type = TypeList<MostDerivedType, typename DerivedToFront<NewNextType>::type>;
};

// For each type in the type list invokes the template struct callable
//
//
// void printTypeInfo(const std::type_info& ti)
// {
//     fmt::print("Type : {}\n", ti.name());
// }
//
// template <typename Type, typename UserType>
// struct PrintValue
// {
//     void operator()(UserType*) const
//     {
//         fmt::print("Value : {}\n", Type::value);
//     }
// };
//
// Below callable struct can be used as DoForEach<TL, PrintTypeInfo>::call()
//  template <typename Type, typename UserType>
//  struct PrintTypeInfo
//  {
//      void operator()(UserType*) const
//      {
//          printTypeInfo(typeid(Type));
//      }
//  };
//
// using T = TL::CreateFromValues<1, 2, 3, 4, 5, 6, 7, 8>::type;
// TL::DoForEach<T, PrintValue>::call<void>(nullptr);
//
template <typename TList, template <typename Type, typename UserType> typename Callable>
struct DoForEach;

template <template <typename Type, typename UserType> typename Callable>
struct DoForEach<NullType, Callable>
{
    template <typename UserType>
    static void call(UserType *userData){};
};
template <typename ThisType, typename NextType, template <typename Type, typename UserType> typename Callable>
struct DoForEach<TypeList<ThisType, NextType>, Callable>
{
    template <typename UserType>
    static void call(UserType *userData)
    {
        Callable<ThisType, UserType>{}(userData);
        DoForEach<NextType, Callable>::call(userData);
    }
};

// Creates typelist from list of types
template <typename... Types>
struct CreateFrom;

template <>
struct CreateFrom<>
{
    using type = NullType;
};
template <typename Type, typename... Types>
struct CreateFrom<Type, Types...>
{
    using type = TypeList<Type, typename CreateFrom<Types...>::type>;
};

template <typename Type>
struct CreateFromSequence;
template <std::integral T, T... Values>
struct CreateFromSequence<std::integer_sequence<T, Values...>>
{
    using type = typename CreateFrom<IntegralToType<T, Values>...>::type;
};
template <uint64_t... Values>
struct CreateFromUInts : public CreateFromSequence<std::integer_sequence<uint64_t, Values...>>
{};
template <int64_t... Values>
struct CreateFromInts : public CreateFromSequence<std::integer_sequence<int64_t, Values...>>
{};

//
template <typename TList, typename... Types>
struct AppendAll;

template <typename TList>
struct AppendAll<TList>
{
    using type = TList;
};
template <typename TList, typename Type, typename... Types>
struct AppendAll<TList, Type, Types...>
{
    using TempType = typename Append<TList, Type>::type;
    using type = typename AppendAll<TempType, Types...>::type;
};

// Applies the typelist to another variadic template type
// Example
//      TL::DoForEach<TL::ApplyAll<TLType, TL::CreateFrom>::type::type, PrintTypeInfo>::call();
// Above example applies the typelist type to CreatFrom which creates a new typelist type which is
// printed here
//
template <typename TList, template <typename...> typename ToType, typename... Types>
struct ApplyAll;

template <template <typename...> typename ToType, typename... Types>
struct ApplyAll<NullType, ToType, Types...>
{
    using type = ToType<Types...>;
};
template <typename ThisType, typename NextType, template <typename...> typename ToType, typename... Types>
struct ApplyAll<TypeList<ThisType, NextType>, ToType, Types...>
{
    using type = typename ApplyAll<NextType, ToType, Types..., ThisType>::type;
};
} // namespace TL