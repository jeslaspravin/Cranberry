/*!
 * \file TemplateTypes.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"

#include <concepts>

// Just some common types
struct NullType
{};

struct EmptyType
{};

// Types to differentiate function overload signatures
template <std::integral T, T Value>
struct IntegralToType
{
    using type = T;
    CONST_EXPR static const T value = Value;
};

template <uint64_t IntValue>
using UIntToType = IntegralToType<uint64_t, IntValue>;
template <int64_t IntValue>
using IntToType = IntegralToType<int64_t, IntValue>;

template <typename Type>
struct TypeToType
{
    using type = Type;
};

template <typename... Types>
struct DependentFalseType : public std::false_type
{};

template <typename... Types>
inline constexpr bool DependentFalseTypeValue = DependentFalseType<Types...>::value;