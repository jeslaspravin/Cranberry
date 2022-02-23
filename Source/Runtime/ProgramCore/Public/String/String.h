/*!
 * \file String.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include <string>
#include <type_traits>
#include <vector>
#include <algorithm>

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "ProgramCoreExports.h"

#define STRINGIFY(...) #__VA_ARGS__

// std::wstring
using BaseString = std::basic_string<TChar, std::char_traits<TChar>, std::allocator<TChar>>;
using StringView = std::basic_string_view<BaseString::value_type, BaseString::traits_type>;
using StringBuffer = std::basic_stringbuf<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using InputStream = std::basic_istream<BaseString::value_type, BaseString::traits_type>;
using OutputStream = std::basic_ostream<BaseString::value_type, BaseString::traits_type>;
using IStringStream = std::basic_istringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using OStringStream = std::basic_ostringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using StringStream = std::basic_stringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;

class String : public BaseString
{
public:
    FORCE_INLINE String() : BaseString() {}
    FORCE_INLINE String(const String& otherString) : BaseString(otherString) {}
    FORCE_INLINE String(const String& otherString, size_type pos, size_type len) : BaseString(otherString, pos, len) {}
    FORCE_INLINE String(const TChar* s, size_type n) : BaseString(s, n) {}
    FORCE_INLINE String(const TChar* s) : BaseString(s) {}
    FORCE_INLINE String(size_type n, TChar c) : BaseString(n, c) {}
    //template<size_t N>
    //CONST_EXPR String(const TChar(&str)[N]) : std::string(str) {}
    FORCE_INLINE String(const BaseString& otherString) : BaseString(otherString) {}
    FORCE_INLINE String(BaseString&& otherString) : BaseString(otherString) {}
    FORCE_INLINE String(BaseString::const_iterator start, BaseString::const_iterator end) : BaseString(start, end) {}
    FORCE_INLINE String(const StringView& strView) : BaseString(strView) {}

    FORCE_INLINE const TChar* getChar() const
    {
        return c_str();
    }

    // Count of String's character code points
    FORCE_INLINE uint64 codeCount() const;

    FORCE_INLINE bool findAny(size_t& outIndex, String& outFoundString, const std::vector<String>& findStrgs, size_t offset = 0, bool fromEnd = false) const
    {
        size_t foundAt = npos;
        for (const String& strg : findStrgs)
        {
            size_t foundAtNew = fromEnd ? rfind(strg, length() - offset) : find(strg, offset);
            if (foundAtNew != npos) {
                outIndex = (foundAt == npos 
                    || (fromEnd 
                        ? foundAt < foundAtNew 
                        : foundAt > foundAtNew)) 
                    ? foundAtNew 
                    : foundAt;
                outFoundString = (foundAt == npos 
                    || (fromEnd 
                        ? foundAt < foundAtNew 
                        : foundAt > foundAtNew)) 
                    ? strg 
                    : outFoundString;
                foundAt = outIndex;
            }
        }
        if (foundAt != npos)
        {
            return true;
        }
        outIndex = npos;
        outFoundString = TCHAR("");
        return false;
    }

    FORCE_INLINE String replaceAllCopy(const String& from, const String& to) const
    {
        String newStr{ this->getChar() };
        size_t replaceAtPos = 0;
        while ((replaceAtPos = newStr.find(from, replaceAtPos)) != npos)
        {
            newStr.replace(replaceAtPos, from.length(), to);
            replaceAtPos += to.length();
        }
        return newStr;
    }

    FORCE_INLINE String& replaceAll(const String& from, const String& to)
    {
        size_t replaceAtPos = 0;
        while ((replaceAtPos = find(from, replaceAtPos)) != npos)
        {
            replace(replaceAtPos, from.length(), to);
            replaceAtPos += to.length();
        }
        return *this;
    }

    FORCE_INLINE bool startsWith(const String& match, bool bMatchCase = true) const
    {
        if (length() < match.length())
            return false;

        if (bMatchCase)
        {
            return match == String(*this, 0, match.length());
        }

        const_iterator it = std::search(cbegin(), cbegin() + match.length(), match.cbegin(), match.cend()
            , [](auto c1, auto c2)
            {
                return std::toupper(c1) == std::toupper(c2);
            });

        return cbegin() == it;
    }

    FORCE_INLINE bool startsWith(TChar match, bool bMatchCase = true) const
    {
        if (bMatchCase)
        {
            return *begin() == match;
        }

        return std::toupper(*begin()) == std::toupper(match);
    }

    FORCE_INLINE bool endsWith(const String& match, bool bMatchCase = true) const
    {
        if (length() < match.length())
            return false;

        if (bMatchCase)
        {
            return match == String(*this, length() - match.length(), match.length());
        }

        const_iterator searchFrom = cbegin() + (length() - match.length());
        const_iterator it = std::search(searchFrom, cend(), match.cbegin(), match.cend()
            , [](auto c1, auto c2)
            {
                return std::toupper(c1) == std::toupper(c2);
            });

        return it == searchFrom;
    }

    FORCE_INLINE String& trimL()
    {
        erase(begin(), std::find_if(begin(), end(),
            [](const TChar& ch)
            {
                return !std::isspace(ch);
            })
        );
        return *this;
    }

    FORCE_INLINE String& trimR()
    {
        erase(std::find_if(rbegin(), rend(),
            [](const TChar& ch)
            {
                return !std::isspace(ch);
            }).base()
            , end()
        );
        return *this;
    }

    FORCE_INLINE String& trim()
    {
        trimL();
        trimR();
        return *this;
    }

    // Removes consequently duplicated chars
    // **NOTE** Only works for characters that can be represented in sizeof(TChar)
    FORCE_INLINE String& trimDuplicates(TChar duplicateChar, uint64 offset = 0)
    {
        uint64 foundAt = find(duplicateChar, offset);
        while (foundAt != npos)
        {
            uint64 searchOffset = 0;
            // Start from character after foundAt char
            while (*(begin() + searchOffset + 1) == duplicateChar)
            {
                searchOffset++;
            }

            // No matter we erased duplicates or not we have to continue from foundAt + 1
            foundAt++;
            if (searchOffset > 0)
            {
                // Since search offset points to last of consequently duplicated chars
                searchOffset++;
                erase(begin() + foundAt, begin() + searchOffset);
            }
            foundAt = find(duplicateChar, foundAt);
        }
        return (*this);
    }

    FORCE_INLINE String trimLCopy() const
    {
        String s(*this);
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](const TChar& ch)
            {
                return !std::isspace(ch);
            })
        );
        return s;
    }

    FORCE_INLINE String trimRCopy() const
    {
        String s(*this);
        s.erase(std::find_if(s.rbegin(), s.rend(),
            [](const TChar& ch)
            {
                return !std::isspace(ch);
            }).base()
                , s.end()
                );
        return s;
    }

    FORCE_INLINE String trimCopy() const
    {
        String s(*this);
        s.trim();
        return s;
    }

    // Removes consequently duplicated chars
    // **NOTE** Only works for characters that can be represented in sizeof(TChar)
    FORCE_INLINE String trimDuplicatesCopy(TChar duplicateChar, uint64 offset = 0)
    {
        String s(*this);
        s.trimDuplicates(duplicateChar, offset);
        return s;
    }

    /*
    * Splits given string into list of line views
    */
    std::vector<StringView> splitLines() const
    {
        std::vector<StringView> outStrs;
        uint64 foundAtPos = 0;
        uint64 offsetPos = 0;
        while ((foundAtPos = find(TCHAR('\n'), offsetPos)) != npos)
        {
            // If previous char is valid and it is carriage return(\r) then we have to consider that as part of CR-LF
            if (foundAtPos != 0 || cbegin()[foundAtPos - 1] == TCHAR('\r'))
            {
                outStrs.emplace_back(StringView(cbegin() + offsetPos, cbegin() + (foundAtPos - 1)));
            }
            else
            {
                // Since offsetPos is end of last separator and foundAtPos is where this separator is found, Whatever in between is what we need
                outStrs.emplace_back(StringView(cbegin() + offsetPos, cbegin() + foundAtPos));
            }
            // Post CR-LF char
            offsetPos = foundAtPos + 1;
        }
        // After final separator the suffix has to be added if there is any char after final CR-LF
        if ((cbegin() + offsetPos) != cend())
        {
            outStrs.emplace_back(StringView(cbegin() + offsetPos, cend()));
        }
        return outStrs;
    }

    template <typename IteratorType>
    static String join(IteratorType begin, IteratorType end, const String&& separator)
    {
        String s;
        if (begin == end)
        {
            return s;
        }
        s += *begin;
        while (++begin != end)
        {
            s += separator;
            s += *begin;
        }
        return s;
    }

    static std::vector<String> split(const String& inStr, const String&& separator)
    {
        std::vector<String> outStrs;
        uint64 foundAtPos = 0;
        uint64 offsetPos = 0;
        while ((foundAtPos = inStr.find(separator, offsetPos)) != npos)
        {
            // Since offsetPos is end of last separator and foundAtPos is where this separator is found, Whatever in between is what we need
            outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cbegin() + foundAtPos));
            offsetPos = foundAtPos + separator.length();
        }
        // After final separator the suffix has to be added
        outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cend()));
        return outStrs;
    }

    template <typename Type>
    FORCE_INLINE static String toString(Type&& value);

    template <typename CharType>
    FORCE_INLINE static const CharType* recurseToNullEnd(const CharType* start)
    {
        const CharType* end = start + 1;
        while (*end != CharType(0))
        {
            end += 1;
        }
        return end;
    }
};


template <>
struct PROGRAMCORE_EXPORT std::hash<String>
{
    NODISCARD size_t operator()(const String& keyval) const noexcept {
        auto stringHash = hash<BaseString>();
        return stringHash(keyval);
    }
};

template<typename T>
struct IsStringUnqualified : std::false_type {};
template<>
struct IsStringUnqualified<String> : std::true_type {};
template<>
struct IsStringUnqualified<BaseString> : std::true_type {};
template<>
struct IsStringUnqualified<TChar> : std::true_type {};
// For char[] arrays
template<typename T, uint32 Num>
struct IsStringUnqualified<T[Num]> : IsStringUnqualified<std::remove_cvref_t<T>> {};
// for char* pointers
template<typename T>
struct IsStringUnqualified<T*> : IsStringUnqualified<std::remove_cvref_t<T>> {};

template<typename T>
struct IsString : IsStringUnqualified<std::remove_cvref_t<T>> {};

template <typename... T>
struct IsStringTypes : std::conjunction<IsString<T>...> {};


template <typename T>
concept StringType = IsString<T>::value;
template <typename T>
concept NonStringType = std::negation_v<IsString<T>>;
template <typename... T>
concept StringTypes = IsStringTypes<T...>::value;


//
// Allows using character literals as template parameters like below
// valPrint<"Test">();
// 
// template <StringLiteral Val>
// void valPrint();
//
template<size_t N>
struct StringLiteral 
{
    CONST_EXPR StringLiteral(const TChar (&str)[N])
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

#include "String/StringHelpers.inl"