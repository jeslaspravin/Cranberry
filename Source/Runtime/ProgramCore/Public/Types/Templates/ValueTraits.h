/*!
 * \file ValueTraits.h
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

template <bool ConditionValue, typename T1, typename T2, T1 Value1, T2 Value2>
struct ConditionalValue
{
    static constexpr T1 value = Value1;
};

template <typename T1, typename T2, T1 Value1, T2 Value2>
struct ConditionalValue<false, T1, T2, Value1, Value2>
{
    static constexpr T2 value = Value2;
};

template <typename T1, typename T2, typename Condition, T1 Value1, T2 Value2>
constexpr std::conditional_t<Condition::value, T1, T2> ConditionalValue_v
    = ConditionalValue<Condition::value, T1, T2, Value1, Value2>::value;