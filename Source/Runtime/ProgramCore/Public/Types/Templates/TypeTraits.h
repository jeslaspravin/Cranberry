/*!
 * \file TypeTraits.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"

#include <concepts>
#include <type_traits>

template <typename FromType, typename ToType, typename = ToType>
struct IsStaticCastable : std::false_type
{};
template <typename FromType, typename ToType>
struct IsStaticCastable<FromType, ToType, decltype(static_cast<ToType>(std::declval<FromType>()))> : std::true_type
{};
template <typename FromType, typename ToType>
concept StaticCastable = IsStaticCastable<FromType, ToType>::value;

// Indexable checks for both compound types and dynamic pointer array and native array
template <typename DataType>
concept IndexableCompoundInternal = requires(DataType val, uint64 idx) {
    typename DataType::value_type;
    {
        val[idx]
    } -> std::convertible_to<typename DataType::value_type>;
    {
        val.size()
    } -> std::convertible_to<SizeT>;
};
template <typename DataType>
concept IndexableCompound = IndexableCompoundInternal<std::remove_cvref_t<DataType>>;

template <typename DataType>
concept IndexableInternal = std::disjunction_v<std::is_array<DataType>, std::is_pointer<DataType>> || IndexableCompound<DataType>;
template <typename DataType>
concept Indexable = IndexableInternal<std::remove_cvref_t<DataType>>;

template <typename DataType>
struct IndexableTypeDeducer
{
    using type = std::conditional_t<std::is_pointer_v<DataType>, std::remove_pointer_t<DataType>, std::remove_all_extents_t<DataType>>;
};
template <IndexableCompound DataType>
struct IndexableTypeDeducer<DataType>
{
    using type = typename DataType::value_type;
};
template <typename DataType>
using IndexableElementType = typename IndexableTypeDeducer<DataType>::type;

template <typename Type, typename... OtherTypes>
concept TypeConvertibleTo = std::conjunction_v<std::is_convertible<Type, OtherTypes>...>;

template <typename Type>
concept NotAnIntegral = std::negation_v<std::is_integral<Type>>;

template <typename Type>
concept TypeIsPointer = std::is_pointer_v<Type>;

// Removes ref(Don't have to remove cv as remove_ptr removes pointer const), then removes ptr, then again
// removes cv & ref of underlying type to get plain type
template <typename Type>
using UnderlyingType = std::remove_cvref_t<std::remove_pointer_t<std::remove_reference_t<Type>>>;
template <typename Type>
using UnderlyingTypeWithConst = std::remove_reference_t<std::remove_pointer_t<std::remove_reference_t<Type>>>;