/*!
 * \file TCharString.h
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
#include "Types/CoreTypes.h"
#include "Types/Platform/PlatformFunctions.h"

#include <string>

template <typename CharType>
using CharStringView = std::basic_string_view<CharType, std::char_traits<CharType>>;

// Some helper functions for null terminated string
// Right now does not handle multiple byte chars as that seems unnecessary to handle that for raw strings
namespace TCharStr
{
template <typename CharType>
NODISCARD constexpr const CharType *recurseToNullEnd(const CharType *start)
{
    while (*start != CharType(0))
    {
        start += 1;
    }
    return start;
}

template <typename CharType>
NODISCARD constexpr SizeT length(const CharType *start)
{
    return recurseToNullEnd(start) - start;
}

template <typename CharType>
NODISCARD constexpr bool empty(const CharType *start)
{
    return length(start) == 0;
}

template <typename CharType>
NODISCARD constexpr bool isEqual(const CharType *lhs, const CharType *rhs)
{
    return CharStringView<CharType>(lhs) == CharStringView<CharType>(rhs);
}

template <typename CharType>
constexpr bool find(CharStringView<CharType> findIn, CharStringView<CharType> findStr, SizeT *outFoundAt = nullptr, SizeT findFrom = 0)
{
    SizeT foundAt = findIn.find(findStr, findFrom);
    if (outFoundAt)
    {
        *outFoundAt = foundAt;
    }
    return foundAt != CharStringView<CharType>::npos;
}

template <typename CharType>
constexpr bool find(CharStringView<CharType> findIn, const CharType findCh, SizeT *outFoundAt = nullptr, SizeT findFrom = 0)
{
    SizeT foundAt = findIn.find(findCh, findFrom);
    if (outFoundAt)
    {
        *outFoundAt = foundAt;
    }
    return foundAt != CharStringView<CharType>::npos;
}

template <typename CharType>
constexpr bool rfind(
    CharStringView<CharType> findIn, CharStringView<CharType> findStr, SizeT *outFoundAt = nullptr,
    SizeT findFrom = CharStringView<CharType>::npos
)
{
    SizeT foundAt = findIn.rfind(findStr, findFrom);
    if (outFoundAt)
    {
        *outFoundAt = foundAt;
    }
    return foundAt != CharStringView<CharType>::npos;
}
template <typename CharType>
constexpr bool
rfind(CharStringView<CharType> findIn, const CharType findCh, SizeT *outFoundAt = nullptr, SizeT findFrom = CharStringView<CharType>::npos)
{
    SizeT foundAt = findIn.rfind(findCh, findFrom);
    if (outFoundAt)
    {
        *outFoundAt = foundAt;
    }
    return foundAt != CharStringView<CharType>::npos;
}

template <typename CharType>
NODISCARD constexpr SizeT findCount(CharStringView<CharType> findIn, const CharType findCh, SizeT findFrom = 0)
{
    SizeT numberOfInst = 0;
    SizeT atPos = 0;
    while (find(findIn, findCh, nullptr, atPos))
    {
        ++numberOfInst;
        ++atPos;
    }
    return numberOfInst;
}
template <typename CharType>
NODISCARD constexpr SizeT findCount(CharStringView<CharType> findIn, CharStringView<CharType> findStr, SizeT findFrom = 0)
{
    SizeT findStrLen = length(findStr);

    SizeT numberOfInst = 0;
    SizeT atPos = 0;
    while (find(findIn, findStr, nullptr, atPos))
    {
        ++numberOfInst;
        atPos += findStrLen;
    }
    return numberOfInst;
}

template <typename CharType>
constexpr void replaceInPlace(CharType *replaceIn, SizeT replaceFrom, SizeT replaceLen, const CharType *replaceWith)
{
    if (replaceLen <= 0 || replaceIn == nullptr || replaceWith == nullptr)
    {
        return;
    }

    // Ensure inputs are valid
#if DEBUG_VALIDATIONS
    SizeT replaceInLen = length(replaceIn);
    SizeT replaceWithLen = length(replaceWith);
    if (replaceFrom >= replaceInLen || (replaceFrom + replaceLen) > replaceInLen || replaceLen >= replaceWithLen)
    {
        return;
    }
#endif

    memcpy(replaceIn + replaceFrom, replaceWith, replaceLen);
}

// Allocated char array must be cleared including null terminated character as count
template <typename CharType, typename AllocatorType>
NODISCARD constexpr CharType *
replace(const CharType *replaceIn, SizeT replaceFrom, SizeT replaceLen, const CharType *replaceWith, AllocatorType &allocator)
{
    if (replaceLen <= 0 || replaceIn == nullptr || replaceWith == nullptr)
    {
        return nullptr;
    }

    SizeT replaceInLen = length(replaceIn);

#if DEBUG_VALIDATIONS
    // Ensure inputs are valid
    if (replaceFrom >= replaceInLen && (replaceFrom + replaceLen) > replaceInLen)
    {
        return nullptr;
    }
#endif

    SizeT replaceWithLen = length(replaceWith);

    SizeT finalLen = replaceInLen - replaceLen + replaceWithLen;
    CharType *retVal = allocator.allocate(finalLen + 1);

    // First section not-replaced
    memcpy(retVal, replaceIn, replaceFrom);
    // Replaced section
    memcpy(retVal + replaceFrom, replaceWith, replaceWithLen);
    // Last section
    memcpy(retVal + replaceFrom + replaceWithLen, replaceIn + replaceFrom + replaceLen, replaceInLen - (replaceLen + replaceFrom));
    // Set null termination
    retVal[finalLen] = 0;
    return retVal;
}

template <typename CharType>
constexpr void replaceAllInPlace(CharType *replaceIn, const CharType *from, const CharType *to)
{
    if (replaceIn == nullptr || from == nullptr || to == nullptr)
    {
        return;
    }

    SizeT replaceFromLen = length(from);
#if DEBUG_VALIDATIONS
    SizeT replaceToLen = length(to);
    if (replaceFromLen != replaceToLen)
    {
        return;
    }
#endif

    SizeT replaceAtPos = 0;
    while (find(replaceIn, from, &replaceAtPos, replaceAtPos))
    {
        replaceInPlace(replaceIn, replaceAtPos, replaceFromLen, to);
        // Since both replace from and to are equal length
        replaceAtPos += replaceFromLen;
    }
}

// Allocated char array must be cleared including null terminated character as count
// Returns null if nothing is replaced, so you can use the original str as it is
template <typename CharType, typename AllocatorType>
NODISCARD constexpr CharType *replaceAll(const CharType *replaceIn, const CharType *from, const CharType *to, AllocatorType &allocator)
{
    if (replaceIn == nullptr || from == nullptr || to == nullptr)
    {
        return nullptr;
    }

    SizeT foundCount = findCount(replaceIn, from);
    if (foundCount == 0)
    {
        return nullptr;
    }

    SizeT replaceFromLen = length(from);
    SizeT replaceToLen = length(to);
    SizeT replaceInLen = length(replaceIn);

    SizeT totalLen = replaceInLen - foundCount * replaceFromLen + foundCount * replaceToLen;
    CharType *retVal = allocator.allocate(totalLen + 1);
    memset(retVal, 0, totalLen + 1);
    // Set initial str
    memcpy(retVal, replaceIn, replaceInLen);

    // Original string char idx to copy from
    SizeT originalStrIdx = 0;
    // Current retVal str idx to start search from
    SizeT retValStrIdx = 0;
    // String found position
    SizeT replaceAtPos = 0;
    while (find(retVal, from, &replaceAtPos, retValStrIdx))
    {
        replaceInPlace(replaceIn, replaceAtPos, replaceToLen, to);
        // Offset original str to char after next replaced from str
        originalStrIdx += (replaceAtPos - retValStrIdx) + replaceFromLen;
        retValStrIdx = replaceAtPos + replaceToLen;
        // Copy rest of str to new search point to replace next from str
        memcpy(retVal + retValStrIdx, replaceIn + originalStrIdx, replaceInLen - originalStrIdx);
    }
    return retVal;
}

template <typename CharType>
constexpr bool startsWith(CharStringView<CharType> matchIn, CharStringView<CharType> match)
{
    if (matchIn.length() < match.length())
    {
        return false;
    }

    return matchIn == match;
}

template <typename CharType>
NODISCARD bool startsWith(CharStringView<CharType> matchIn, CharStringView<CharType> match, bool bMatchCase)
{
    if (matchIn.length() < match.length())
    {
        return false;
    }

    if (bMatchCase)
    {
        return matchIn == match;
    }

    auto it = std::search(
        matchIn.cbegin(), matchIn.cend(), match.cbegin(), match.cend(),
        [](const CharType c1, const CharType c2)
        {
            return PlatformFunctions::toUpper(c1) == PlatformFunctions::toUpper(c2);
        }
    );

    return matchIn.cbegin() == it;
}

template <typename CharType>
NODISCARD constexpr bool startsWith(CharStringView<CharType> matchIn, const CharType match)
{
    return matchIn[0] == match;
}
template <typename CharType>
NODISCARD bool startsWith(CharStringView<CharType> matchIn, const CharType match, bool bMatchCase)
{
    if (bMatchCase)
    {
        return startsWith(matchIn, match);
    }
    return PlatformFunctions::toUpper(matchIn[0]) == PlatformFunctions::toUpper(match);
}

template <typename CharType>
NODISCARD bool endsWith(CharStringView<CharType> matchIn, CharStringView<CharType> match, bool bMatchCase)
{
    if (matchIn.length() < match.length())
    {
        return false;
    }

    if (bMatchCase)
    {
        return matchIn == match;
    }

    auto it = std::search(
        matchIn.cbegin(), matchIn.cend(), match.cbegin(), match.cend(),
        [](const CharType c1, const CharType c2)
        {
            return PlatformFunctions::toUpper(c1) == PlatformFunctions::toUpper(c2);
        }
    );

    return matchIn.cbegin() == it;
}

template <typename CharType>
NODISCARD constexpr CharStringView<CharType> trimL(CharStringView<CharType> strView)
{
    auto itr = std::find_if(
        strView.cbegin(), strView.cend(),
        [](const CharType ch)
        {
            return !std::isspace(ch);
        }
    );

    if (itr == strView.cend())
    {
        return strView;
    }

    return CharStringView<CharType>(itr, strView.cend());
}

template <typename CharType>
NODISCARD constexpr CharStringView<CharType> trimR(CharStringView<CharType> strView)
{
    auto itr = std::find_if(
        strView.crbegin(), strView.crend(),
        [](const CharType ch)
        {
            return !std::isspace(ch);
        }
    );

    if (itr == strView.cend())
    {
        return strView;
    }

    return CharStringView<CharType>(strView.cbegin(), itr);
}

template <typename CharType>
NODISCARD constexpr CharStringView<CharType> trim(CharStringView<CharType> strView)
{
    CharStringView<CharType> retView = trimL(strView);
    return trimR(retView);
}

template <typename CharType>
NODISCARD constexpr std::vector<CharStringView<CharType>> splitLines(CharStringView<CharType> str)
{
    std::vector<CharStringView<CharType>> outStrs;

    uint64 foundAtPos = 0;
    uint64 offsetPos = 0;
    while (find(str, '\n', &foundAtPos, offsetPos))
    {
        // If previous char is valid and it is carriage return(\r) then we have to consider that as
        // part of CR-LF
        if (foundAtPos != 0 || str[foundAtPos - 1] == TCHAR('\r'))
        {
            outStrs.emplace_back(str.substr(offsetPos, foundAtPos - offsetPos - 1));
        }
        else
        {
            // Since offsetPos is end of last separator and foundAtPos is where this separator is
            // found, Whatever in between is what we need
            outStrs.emplace_back(str.substr(offsetPos, foundAtPos - offsetPos));
        }
        // Post CR-LF char
        offsetPos = foundAtPos + 1;
    }
    // After final separator the suffix has to be added if there is any char after final CR-LF
    if (offsetPos != str.length())
    {
        outStrs.emplace_back(str.substr(offsetPos));
    }
    return outStrs;
}

template <typename CharType>
NODISCARD constexpr std::vector<CharStringView<CharType>> split(CharStringView<CharType> str, CharStringView<CharType> separator)
{
    SizeT separatorLen = separator.length();

    std::vector<CharStringView<CharType>> outStrs;

    uint64 foundAtPos = 0;
    uint64 offsetPos = 0;
    while (find(str, separator, &foundAtPos, offsetPos))
    {
        outStrs.emplace_back(str.substr(offsetPos, foundAtPos - offsetPos));
        offsetPos = foundAtPos + separatorLen;
    }
    // After final separator the suffix has to be added if there is any char after final match
    if (offsetPos != str.length())
    {
        outStrs.emplace_back(str.substr(offsetPos));
    }
    return outStrs;
}
} // namespace TCharStr

namespace TCharUtils
{
template <typename CharType, std::integral OutType>
constexpr bool parseHex(OutType &outVal, CharStringView<CharType> strView)
{
    if (strView.empty())
    {
        return false;
    }

    // If starting with 0x then skip them
    if (*strView.cbegin() == '0' && strView.cbegin()[1] == 'x')
    {
        strView.remove_prefix(2);
    }

    OutType retVal = 0;
    for (auto itr = strView.cbegin(); itr != strView.cend(); ++itr)
    {
        CharType diff = *itr - '0';
        // If not a digit then check for accepted char
        if (diff < 0 || diff > 9)
        {
            CharType subtrahend = (*itr >= 'a') ? 'a' : 'A';
            // Add 10 to skip to 10 in hex
            diff = *itr - subtrahend + 10;
            // If not in valid alphabet range then stop parsing
            if (diff < 10 || diff > 15)
            {
                return false;
            }
        }
        retVal = retVal * 16 + diff;
    }

    outVal = retVal;
    return true;
}

template <typename CharType, std::integral OutType>
constexpr bool parseHex(OutType &outVal, const CharType *str)
{
    return parseHex(outVal, CharStringView<CharType>(str, TCharStr::length(str)));
}
} // namespace TCharUtils