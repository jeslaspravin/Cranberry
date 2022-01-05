/*!
 * \file TypeTraits.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <type_traits>
#include <concepts>

// Indexable checks for dynamic pointer array and native array only
template <typename DataType>
using IsIndexable = std::disjunction<std::is_array<DataType>, std::is_pointer<DataType>>;
template <typename DataType>
using IndexableElementType = std::conditional_t<std::is_pointer_v<DataType>, std::remove_pointer_t<DataType>, std::remove_all_extents_t<DataType>>;

// Indexable checks for both compound types and dynamic pointer array and native array
template <typename DataType>
concept IndexableCompound = requires(DataType& val, uint64 idx)
    {
        DataType::value_type;
        { val[idx] } -> std::convertible_to<typename DataType::value_type>;
    };
template <typename DataType>
concept Indexable = std::disjunction_v<std::is_array<DataType>, std::is_pointer<DataType>> || IndexableCompound<DataType>;

// Removes ref(Don't have to remove cv as remove_ptr removes pointer const), then removes ptr, then again removes cv & ref of underlying type to get plain type
template <typename Type>
using UnderlyingType = std::remove_cvref_t<std::remove_pointer_t<std::remove_reference_t<Type>>>;
template <typename Type>
using UnderlyingTypeWithConst = std::remove_reference_t<std::remove_pointer_t<std::remove_reference_t<Type>>>;