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
    CONST_EXPR AStringLiteral(const AChar (&str)[N]) { std::copy_n(str, N, value); }

    AChar value[N];
};
template <SizeT N>
struct WStringLiteral
{
    CONST_EXPR WStringLiteral(const WChar (&str)[N]) { std::copy_n(str, N, value); }

    WChar value[N];
};

template <SizeT N>
struct StringLiteral
{
    CONST_EXPR StringLiteral(const TChar (&str)[N]) { std::copy_n(str, N, value); }

    TChar value[N];
};

template <StringLiteral StoreValue>
struct StringLiteralStore
{
    using LiteralType = decltype(StoreValue);
    CONST_EXPR static const LiteralType Literal = StoreValue;

    CONST_EXPR const TChar *getChar() const { return StoreValue.value; }

    operator String () const { return { StoreValue.value }; }

    String toString() const { return { StoreValue.value }; }
};