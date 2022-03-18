/*!
 * \file TCharString.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Memory/Memory.h"

#include <string>

 //
 // Allows using character literals as template parameters like below
 // valPrint<"Test">();
 // 
 // template <StringLiteral Val>
 // void valPrint();
 //
template<String::size_type N>
struct StringLiteral
{
    CONST_EXPR StringLiteral(const TChar(&str)[N])
    {
        std::copy_n(str, N, value);
    }

    TChar value[N];
};

template <StringLiteral StoreValue>
struct StringLiteralStore
{
    using LiteralType = decltype(StoreValue);
    CONST_EXPR static const LiteralType Literal = StoreValue;

    CONST_EXPR const TChar* getChar() const
    {
        return StoreValue.value;
    }

    operator String() const
    {
        return { StoreValue.value };
    }

    String toString() const
    {
        return { StoreValue.value };
    }
};

// Some helper functions for null terminated string
// Right now does not handle multiple byte chars as that seems unnecessary to handle that for raw strings
namespace TCharStr
{
    template <typename CharType>
    using CharStringView = std::basic_string_view<CharType, std::char_traits<CharType>>;

    template <typename CharType>
    NODISCARD CONST_EXPR const CharType* recurseToNullEnd(const CharType* start)
    {
        const CharType* end = start + 1;
        while (*end != CharType(0))
        {
            end += 1;
        }
        return end;
    }

    template <typename CharType>
    NODISCARD CONST_EXPR SizeT length(const CharType* start)
    {
        return recurseToNullEnd(start) - start;
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR bool find(const CharType* findIn, const CharType* findStr, SizeT* outFoundAt = nullptr, SizeT findFrom = 0)
    {
        StringViewType strView(findIn);
        SizeT foundAt = strView.find(findStr, findFrom);
        if (outFoundAt)
        {
            *outFoundAt = foundAt;
        }
        return foundAt != StringViewType::npos;
    }
    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR bool find(const CharType* findIn, const CharType findCh, SizeT* outFoundAt = nullptr, SizeT findFrom = 0)
    {
        StringViewType strView(findIn);
        SizeT foundAt = strView.find(findCh, findFrom);
        if (outFoundAt)
        {
            *outFoundAt = foundAt;
        }
        return foundAt != StringViewType::npos;
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR bool rfind(const CharType* findIn, const CharType* findStr, SizeT* outFoundAt = nullptr, SizeT findFrom = 0)
    {
        StringViewType strView(findIn);
        SizeT foundAt = strView.rfind(findStr, findFrom);
        if (outFoundAt)
        {
            *outFoundAt = foundAt;
        }
        return foundAt != StringViewType::npos;
    }
    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR bool rfind(const CharType* findIn, const CharType findCh, SizeT* outFoundAt = nullptr, SizeT findFrom = 0)
    {
        StringViewType strView(findIn);
        SizeT foundAt = strView.rfind(findCh, findFrom);
        if (outFoundAt)
        {
            *outFoundAt = foundAt;
        }
        return foundAt != StringViewType::npos;
    }

    template <typename CharType>
    NODISCARD CONST_EXPR SizeT findCount(const CharType* findIn, const CharType findCh, SizeT findFrom = 0)
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
    NODISCARD CONST_EXPR SizeT findCount(const CharType* findIn, const CharType* findStr, SizeT findFrom = 0)
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

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR void replaceInPlace(CharType* replaceIn, SizeT replaceFrom, SizeT replaceLen,const CharType* replaceWith)
    {
        if (replaceLen <= 0 || replaceIn == nullptr || replaceWith == nullptr)
        {
            return;
        }

        // Ensure inputs are valid
#if DEBUG_BUILD
        SizeT replaceInLen = length(replaceIn);
        SizeT replaceWithLen = length(replaceWith);
        debugAssert(replaceFrom < replaceInLen && (replaceFrom + replaceLen) <= replaceInLen);
        debugAssert(replaceLen < replaceWithLen);
#endif

        CBEMemory::memCopy(replaceIn + replaceFrom, replaceWith, replaceLen);
    }

    // Allocated char array must be cleared including null terminated character as count
    template <typename CharType, typename AllocatorType>
    NODISCARD CONST_EXPR CharType* replace(const CharType* replaceIn, SizeT replaceFrom, SizeT replaceLen, const CharType* replaceWith, AllocatorType& allocator)
    {
        if (replaceLen <= 0 || replaceIn == nullptr || replaceWith == nullptr)
        {
            return;
        }

        SizeT replaceInLen = length(replaceIn);
        // Ensure inputs are valid
        debugAssert(replaceFrom < replaceInLen && (replaceFrom + replaceLen) <= replaceInLen);

        SizeT replaceWithLen = length(replaceWith);

        SizeT finalLen = replaceInLen - replaceLen + replaceWithLen;
        CharType* retVal = allocator.allocate(finalLen + 1);

        // First section not-replaced
        CBEMemory::memCopy(retVal, replaceIn, replaceFrom);
        // Replaced section
        CBEMemory::memCopy(retVal + replaceFrom, replaceWith, replaceWithLen);
        // Last section
        CBEMemory::memCopy(retVal + replaceFrom + replaceWithLen, replaceIn + replaceFrom + replaceLen, replaceInLen - (replaceLen + replaceFrom));
        // Set null termination
        retVal[finalLen] = 0;
        return retVal;
    }

    template <typename CharType>
    CONST_EXPR void replaceAllInPlace(CharType* replaceIn, const CharType* from, const CharType* to)
    {
        if (replaceIn == nullptr || from == nullptr || to == nullptr)
        {
            return;
        }

        SizeT replaceFromLen = length(from);
#if DEBUG_BUILD
        SizeT replaceToLen = length(to);
        debugAssert(replaceFromLen == replaceToLen);
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
    NODISCARD CONST_EXPR CharType* replaceAll(const CharType* replaceIn, const CharType* from, const CharType* to, AllocatorType& allocator)
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
        CharType* retVal = allocator.allocate(totalLen + 1);
        CBEMemory::memZero(retVal, totalLen + 1);
        // Set initial str
        CBEMemory::memCopy(retVal, replaceIn, replaceInLen);

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
            CBEMemory::memCopy(retVal + retValStrIdx, replaceIn + originalStrIdx, replaceInLen - originalStrIdx);
        }
        return retVal;
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    CONST_EXPR bool startsWith(const CharType* matchIn, const CharType* match)
    {
        SizeT matchInLen = length(matchIn);
        SizeT matchLen = length(match);

        if (matchInLen < matchLen)
            return false;

        StringViewType matchInView(matchIn, 0, matchLen);
        StringViewType matchView(match, 0, matchLen);

        return  matchInView == matchView;
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    NODISCARD bool startsWith(const CharType* matchIn, const CharType* match, bool bMatchCase)
    {
        SizeT matchInLen = length(matchIn);
        SizeT matchLen = length(match);

        if (matchInLen < matchLen)
            return false;

        StringViewType matchInView(matchIn, 0, matchLen);
        StringViewType matchView(match, 0, matchLen);
        if (bMatchCase)
        {
            return  matchInView == matchView;
        }

        auto it = std::search(matchInView.cbegin(), matchInView.cend(), matchView.cbegin(), matchView.cend()
            , [](CharType c1, CharType c2)
            {
                return PlatformFunctions::toUpper(c1) == PlatformFunctions::toUpper(c2);
            });

        return matchInView.cbegin() == it;
    }

    template <typename CharType>
    NODISCARD CONST_EXPR bool startsWith(const CharType* matchIn, CharType match)
    {
        return *matchIn == match;
    }
    template <typename CharType>
    NODISCARD bool startsWith(const CharType* matchIn, CharType match, bool bMatchCase)
    {
        if (bMatchCase)
        {
            return startsWith(matchIn, match);
        }
        return PlatformFunctions::toUpper(*matchIn) == PlatformFunctions::toUpper(match);
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    NODISCARD bool endsWith(const CharType* matchIn, const CharType* match, bool bMatchCase)
    {
        SizeT matchInLen = length(matchIn);
        SizeT matchLen = length(match);

        if (matchInLen < matchLen)
            return false;

        StringViewType matchInView(matchIn, matchInLen - matchLen, matchInLen);
        StringViewType matchView(match, 0, matchLen);
        if (bMatchCase)
        {
            return  matchInView == matchView;
        }

        auto it = std::search(matchInView.cbegin(), matchInView.cend(), matchView.cbegin(), matchView.cend()
            , [](CharType c1, CharType c2)
            {
                return PlatformFunctions::toUpper(c1) == PlatformFunctions::toUpper(c2);
            });

        return matchInView.cbegin() == it;
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    NODISCARD CONST_EXPR StringViewType trimL(const CharType* str)
    {
        StringViewType strView(str);
        auto itr = std::find_if(strView.cbegin(), strView.cend()
            , [](CharType ch)
            {
                return !std::isspace(ch);
            });

        if (itr == strView.cend())
        {
            return strView;
        }

        return StringViewType(&*itr);
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    NODISCARD CONST_EXPR StringViewType trimR(const CharType* str)
    {
        StringViewType strView(str);
        auto itr = std::find_if(strView.crbegin(), strView.crend()
            , [](CharType ch)
            {
                return !std::isspace(ch);
            });

        if (itr == strView.cend())
        {
            return strView;
        }

        return StringViewType(str, &*itr);
    }

    template <typename CharType, typename StringViewType = CharStringView<CharType>>
    NODISCARD CONST_EXPR StringViewType trim(const CharType* str)
    {
        StringViewType retView = trimL(str);
        return trimR(&*retView.cbegin());
    }
}