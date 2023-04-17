/*!
 * \file StringLiteral.h
 *
 * \author Jeslas
 * \date April 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

//
// Allows using character literals as template parameters like below
// valPrint<"Test">();
//
// template <StringLiteral Val>
// void valPrint();
//
template <SizeT N>
struct AStringLiteral
{
    constexpr static const SizeT CountWithNull = N;
    constexpr static const SizeT Count = CountWithNull - 1;
    using value_type = AChar;
    using element_type = AChar;
    constexpr static const SizeT BytesCountWithNull = CountWithNull * sizeof(element_type);
    constexpr static const SizeT BytesCount = Count * sizeof(element_type);

    constexpr AStringLiteral(const value_type (&str)[N]) { std::copy_n(str, N, value); }

    value_type value[N];
};
template <SizeT N>
struct WStringLiteral
{
    constexpr static const SizeT CountWithNull = N;
    constexpr static const SizeT Count = CountWithNull - 1;
    using value_type = WChar;
    using element_type = WChar;
    constexpr static const SizeT BytesCountWithNull = CountWithNull * sizeof(element_type);
    constexpr static const SizeT BytesCount = Count * sizeof(element_type);

    constexpr WStringLiteral(const value_type (&str)[N]) { std::copy_n(str, N, value); }

    value_type value[N];
};

template <SizeT N>
struct StringLiteral
{
    constexpr static const SizeT CountWithNull = N;
    constexpr static const SizeT Count = CountWithNull - 1;
    using value_type = TChar;
    using element_type = TChar;
    constexpr static const SizeT BytesCountWithNull = CountWithNull * sizeof(element_type);
    constexpr static const SizeT BytesCount = Count * sizeof(element_type);

    constexpr StringLiteral(const value_type (&str)[N]) { std::copy_n(str, N, value); }

    value_type value[N];
};

template <StringLiteral StoreValue>
struct StringLiteralStore
{
    using LiteralType = decltype(StoreValue);
    using value_type = LiteralType::value_type;
    using element_type = LiteralType::element_type;
    constexpr static const SizeT CountWithNull = LiteralType::CountWithNull;
    constexpr static const SizeT Count = LiteralType::Count;
    constexpr static const SizeT BytesCountWithNull = LiteralType::BytesCountWithNull;
    constexpr static const SizeT BytesCount = LiteralType::BytesCount;

    constexpr static const LiteralType Literal = StoreValue;

    constexpr const value_type *getChar() const { return StoreValue.value; }
    operator String () const { return { StoreValue.value }; }
    String toString() const { return { StoreValue.value }; }
};