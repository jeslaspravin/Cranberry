/*!
 * \file xxHashInclude.hpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "String/TCharString.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#if BIG_ENDIAN
#define XXH32_BIG_ENDIAN
#include "Types/xxHash/xxh32.hpp"
#undef XXH32_BIG_ENDIAN

#define XXH64_BIG_ENDIAN BIG_ENDIAN
#include "Types/xxHash/xxh64.hpp"
#undef XXH64_BIG_ENDIAN

#else // BIG_ENDIAN
#include "Types/xxHash/xxh32.hpp"
#include "Types/xxHash/xxh64.hpp"
#endif // BIG_ENDIAN

namespace xxHash
{
template <typename CharType>
CONST_EXPR uint32 hashString(const CharType *str, uint32 seed)
{
    return xxh32::hash(str, uint32(TCharStr::length(str) * sizeof(CharType)), seed);
}
template <typename CharType>
CONST_EXPR uint64 hashString(const CharType *str, uint64 seed)
{
    return xxh64::hash(str, uint32(TCharStr::length(str) * sizeof(CharType)), seed);
}
template <typename CharType>
CONST_EXPR uint32 hashString(const CharType *str, uint32 len, uint32 seed)
{
    return xxh32::hash(str, uint32(len * sizeof(CharType)), seed);
}
template <typename CharType>
CONST_EXPR uint64 hashString(const CharType *str, uint64 len, uint64 seed)
{
    return xxh64::hash(str, uint32(len * sizeof(CharType)), seed);
}

FORCE_INLINE uint32 hashString(const String &str, uint32 seed)
{
    if CONST_EXPR (String::bIsWide)
    {
        return hashString(TCHAR_TO_UTF8(str.getChar()), seed);
    }
    else
    {
        return xxh32::hash(str.getChar(), uint32(str.length() * sizeof(String::value_type)), seed);
    }
}
FORCE_INLINE uint64 hashString(const String &str, uint64 seed)
{
    if CONST_EXPR (String::bIsWide)
    {
        return hashString(TCHAR_TO_UTF8(str.getChar()), seed);
    }
    else
    {
        return xxh64::hash(str.getChar(), uint32(str.length() * sizeof(String::value_type)), seed);
    }
}
} // namespace xxHash