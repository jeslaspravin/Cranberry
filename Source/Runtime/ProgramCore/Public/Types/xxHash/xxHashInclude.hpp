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
#include "String/StringLiteral.h"
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

constexpr uint32 hashString(const AChar *str, uint32 len, uint32 seed) { return xxh32::hash(str, len, seed); }
constexpr uint64 hashString(const AChar *str, uint64 len, uint64 seed) { return xxh64::hash(str, len, seed); }
inline uint32 hashString(const WChar *str, uint32 len, uint32 seed)
{
    return xxh32::hash(reinterpret_cast<const AChar *>(str), uint32(len * sizeof(WChar)), seed);
}
inline uint64 hashString(const WChar *str, uint64 len, uint64 seed)
{
    return xxh64::hash(reinterpret_cast<const AChar *>(str), len * sizeof(WChar), seed);
}

template <StringLiteral str>
consteval uint32 hashString(uint32 seed)
{
    return xxh32::hash(std::bit_cast<AStringLiteral<decltype(str)::BytesCountWithNull>>(str).value, uint32(decltype(str)::BytesCount), seed);
}
template <StringLiteral str>
consteval uint64 hashString(uint64 seed)
{
    return xxh64::hash(std::bit_cast<AStringLiteral<decltype(str)::BytesCountWithNull>>(str).value, decltype(str)::BytesCount, seed);
}

template <typename CharType>
constexpr uint32 hashString(const CharType *str, uint32 seed)
{
    return hashString(str, uint32(TCharStr::length(str)), seed);
}
template <typename CharType>
constexpr uint64 hashString(const CharType *str, uint64 seed)
{
    return hashString(str, TCharStr::length(str), seed);
}

template <typename CharType>
constexpr uint32 hashString(CharStringView<CharType> strView, uint32 seed)
{
    return hashString(&strView[0], uint32(strView.length()), seed);
}
template <typename CharType>
constexpr uint64 hashString(CharStringView<CharType> strView, uint64 seed)
{
    return hashString(&strView[0], strView.length(), seed);
}

} // namespace xxHash