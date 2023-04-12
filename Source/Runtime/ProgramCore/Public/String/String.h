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

#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/Templates/TemplateTypes.h"

#define STRINGIFY(...) #__VA_ARGS__

// std::string
using BaseString = std::basic_string<TChar, std::char_traits<TChar>, std::allocator<TChar>>;
using StringView = std::basic_string_view<BaseString::value_type, BaseString::traits_type>;
using StringBuffer = std::basic_stringbuf<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using InputStream = std::basic_istream<BaseString::value_type, BaseString::traits_type>;
using OutputStream = std::basic_ostream<BaseString::value_type, BaseString::traits_type>;
using IStringStream = std::basic_istringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using OStringStream = std::basic_ostringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;
using StringStream = std::basic_stringstream<BaseString::value_type, BaseString::traits_type, BaseString::allocator_type>;

#ifdef __cpp_lib_constexpr_string
#define HAS_STRING_CONSTEXPR __cpp_lib_constexpr_string >= 201907L
#define STRING_FUNCQUALIFIER constexpr
#else
#define HAS_STRING_CONSTEXPR 0
#define STRING_FUNCQUALIFIER FORCE_INLINE
#endif

class String : public BaseString
{
public:
    STRING_FUNCQUALIFIER String()
        : BaseString()
    {}
    STRING_FUNCQUALIFIER String(const String &otherString)
        : BaseString(otherString)
    {}
    STRING_FUNCQUALIFIER String(const String &otherString, size_type pos, size_type len)
        : BaseString(otherString, pos, len)
    {}
    STRING_FUNCQUALIFIER String(const TChar *s, size_type n)
        : BaseString(s, n)
    {}
    STRING_FUNCQUALIFIER String(const TChar *s)
        : BaseString(s)
    {}
    STRING_FUNCQUALIFIER String(size_type n, TChar c)
        : BaseString(n, c)
    {}
    // template<size_type N>
    // CONST_EXPR String(const TChar(&str)[N]) : std::string(str) {}
    STRING_FUNCQUALIFIER String(const BaseString &otherString)
        : BaseString(otherString)
    {}
    STRING_FUNCQUALIFIER String(BaseString &&otherString)
        : BaseString(otherString)
    {}
    STRING_FUNCQUALIFIER String(BaseString::const_iterator start, BaseString::const_iterator end)
        : BaseString(start, end)
    {}
    STRING_FUNCQUALIFIER String(const StringView &strView)
        : BaseString(strView)
    {}

    STRING_FUNCQUALIFIER const TChar *getChar() const { return c_str(); }

    // Count of String's character code points
    FORCE_INLINE uint64 codeCount() const;

    STRING_FUNCQUALIFIER bool
    findAny(size_type &outIndex, String &outFoundString, const std::vector<String> &findStrgs, size_type offset = 0, bool fromEnd = false) const
    {
        size_type foundAt = npos;
        for (const String &strg : findStrgs)
        {
            size_type foundAtNew = fromEnd ? rfind(strg, length() - offset) : find(strg, offset);
            if (foundAtNew != npos)
            {
                outIndex = (foundAt == npos || (fromEnd ? foundAt < foundAtNew : foundAt > foundAtNew)) ? foundAtNew : foundAt;
                outFoundString = (foundAt == npos || (fromEnd ? foundAt < foundAtNew : foundAt > foundAtNew)) ? strg : outFoundString;
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

    STRING_FUNCQUALIFIER String replaceAllCopy(const String &from, const String &to) const
    {
        String newStr{ this->getChar() };
        size_type replaceAtPos = 0;
        while ((replaceAtPos = newStr.find(from, replaceAtPos)) != npos)
        {
            newStr.replace(replaceAtPos, from.length(), to);
            replaceAtPos += to.length();
        }
        return newStr;
    }

    STRING_FUNCQUALIFIER String &replaceAll(const String &from, const String &to)
    {
        size_type replaceAtPos = 0;
        while ((replaceAtPos = find(from, replaceAtPos)) != npos)
        {
            replace(replaceAtPos, from.length(), to);
            replaceAtPos += to.length();
        }
        return *this;
    }

    STRING_FUNCQUALIFIER bool isEqual(const String &match, bool bMatchCase = true) const
    {
        if (length() != match.length())
        {
            return false;
        }

        if (bMatchCase)
        {
            return match == *this;
        }

        const_iterator it = std::search(
            cbegin(), cend(), match.cbegin(), match.cend(),
            [](auto c1, auto c2)
            {
                return std::toupper(c1) == std::toupper(c2);
            }
        );

        return cbegin() == it;
    }

    STRING_FUNCQUALIFIER bool startsWith(const String &match, bool bMatchCase = true) const
    {
        if (length() < match.length())
        {
            return false;
        }

        if (bMatchCase)
        {
            return StringView(match) == StringView(cbegin(), cbegin() + match.length());
        }

        const_iterator it = std::search(
            cbegin(), cbegin() + match.length(), match.cbegin(), match.cend(),
            [](auto c1, auto c2)
            {
                return std::toupper(c1) == std::toupper(c2);
            }
        );

        return cbegin() == it;
    }

    STRING_FUNCQUALIFIER bool startsWith(TChar match, bool bMatchCase = true) const
    {
        if (length() == 0)
        {
            return match == 0;
        }

        if (bMatchCase)
        {
            return *begin() == match;
        }

        return std::toupper(*begin()) == std::toupper(match);
    }

    STRING_FUNCQUALIFIER bool endsWith(const String &match, bool bMatchCase = true) const
    {
        if (length() < match.length())
        {
            return false;
        }

        if (bMatchCase)
        {
            return StringView(match) == StringView(cbegin() + (length() - match.length()), cbegin() + match.length());
        }

        const_iterator searchFrom = cbegin() + (length() - match.length());
        const_iterator it = std::search(
            searchFrom, cend(), match.cbegin(), match.cend(),
            [](auto c1, auto c2)
            {
                return std::toupper(c1) == std::toupper(c2);
            }
        );

        return it == searchFrom;
    }

    STRING_FUNCQUALIFIER bool endsWith(TChar match, bool bMatchCase = true) const
    {
        if (length() == 0)
        {
            return match == 0;
        }
        auto endItr = (end() - 1);
        if (bMatchCase)
        {
            return *endItr == match;
        }

        return std::toupper(*endItr) == std::toupper(match);
    }

    STRING_FUNCQUALIFIER String &trimL()
    {
        erase(
            begin(), std::find_if(
                         begin(), end(),
                         [](const TChar &ch)
                         {
                             return !std::isspace(ch);
                         }
                     )
        );
        return *this;
    }

    STRING_FUNCQUALIFIER String &trimR()
    {
        erase(
            std::find_if(
                rbegin(), rend(),
                [](const TChar &ch)
                {
                    return !std::isspace(ch);
                }
            ).base(),
            end()
        );
        return *this;
    }

    STRING_FUNCQUALIFIER String &trim()
    {
        trimL();
        trimR();
        return *this;
    }

    // Removes consequently duplicated chars
    // **NOTE** Only works for characters that can be represented in sizeof(TChar)
    STRING_FUNCQUALIFIER String &trimDuplicates(TChar duplicateChar, uint64 offset = 0)
    {
        uint64 foundAt = find(duplicateChar, offset);
        while (foundAt != npos)
        {
            // Start from character after foundAt char
            auto strItr = cbegin() + foundAt;
            while ((strItr + 1) != cend() && (*(strItr + 1) == duplicateChar))
            {
                strItr++;
            }
            uint64 searchOffset = strItr - cbegin();

            // No matter we erased duplicates or not we have to continue from foundAt + 1
            foundAt++;
            if (searchOffset >= foundAt)
            {
                // Since search offset points to last of consequently duplicated chars
                searchOffset++;
                erase(begin() + foundAt, begin() + searchOffset);
            }
            foundAt = find(duplicateChar, foundAt);
        }
        return (*this);
    }

    STRING_FUNCQUALIFIER String trimLCopy() const
    {
        String s(*this);
        s.erase(
            s.begin(), std::find_if(
                           s.begin(), s.end(),
                           [](const TChar &ch)
                           {
                               return !std::isspace(ch);
                           }
                       )
        );
        return s;
    }

    STRING_FUNCQUALIFIER String trimRCopy() const
    {
        String s(*this);
        s.erase(
            std::find_if(
                s.rbegin(), s.rend(),
                [](const TChar &ch)
                {
                    return !std::isspace(ch);
                }
            ).base(),
            s.end()
        );
        return s;
    }

    STRING_FUNCQUALIFIER String trimCopy() const
    {
        String s(*this);
        s.trim();
        return s;
    }

    // Removes consequently duplicated chars
    // **NOTE** Only works for characters that can be represented in sizeof(TChar)
    STRING_FUNCQUALIFIER String trimDuplicatesCopy(TChar duplicateChar, uint64 offset = 0) const
    {
        String s(*this);
        s.trimDuplicates(duplicateChar, offset);
        return s;
    }

    // Remove count number of characters from start of string
    STRING_FUNCQUALIFIER String &eraseL(uint64 count) noexcept
    {
        if (count > length())
        {
            clear();
            return *this;
        }
        erase(0, count);
        return *this;
    }
    // Remove count number of characters from end of string
    STRING_FUNCQUALIFIER String &eraseR(uint64 count) noexcept
    {
        if (count > length())
        {
            clear();
            return *this;
        }
        erase(length() - count, count);
        return *this;
    }
    // Remove count number of characters from start of string
    STRING_FUNCQUALIFIER String eraseLCopy(uint64 count) const noexcept
    {
        if (count > length())
        {
            return {};
        }
        return String(cbegin() + count, cend());
    }
    // Remove count number of characters from end of string
    STRING_FUNCQUALIFIER String eraseRCopy(uint64 count) const noexcept
    {
        if (count > length())
        {
            return {};
        }
        return String(cbegin(), cbegin() + (length() - count));
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
            // If previous char is valid and it is carriage return(\r) then we have to consider that
            // as part of CR-LF
            if (foundAtPos != 0 || cbegin()[foundAtPos - 1] == TCHAR('\r'))
            {
                outStrs.emplace_back(StringView(cbegin() + offsetPos, cbegin() + (foundAtPos - 1)));
            }
            else
            {
                // Since offsetPos is end of last separator and foundAtPos is where this
                // separator is found, Whatever in between is what we need
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

    void toUpper()
    {
        for (TChar &ch : (*this))
        {
            ch = TChar(std::toupper(ch));
        }
    }
    void toLower()
    {
        for (TChar &ch : (*this))
        {
            ch = TChar(std::tolower(ch));
        }
    }
    String toUpperCopy() const
    {
        String str = *this;
        str.toUpper();
        return str;
    }
    String toLowerCopy() const
    {
        String str = *this;
        str.toLower();
        return str;
    }

    template <typename IteratorType, typename StrType>
    static String join(IteratorType begin, IteratorType end, StrType &&separator)
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

    static std::vector<String> split(const String &inStr, const String &&separator)
    {
        std::vector<String> outStrs;
        uint64 foundAtPos = 0;
        uint64 offsetPos = 0;
        while ((foundAtPos = inStr.find(separator, offsetPos)) != npos)
        {
            // Since offsetPos is end of last separator and foundAtPos is where this separator is
            // found, Whatever in between is what we need
            outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cbegin() + foundAtPos));
            offsetPos = foundAtPos + separator.length();
        }
        // After final separator the suffix has to be added
        outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cend()));
        return outStrs;
    }

    template <typename Type>
    STRING_FUNCQUALIFIER static String toString(Type &&value);
};

template <>
struct PROGRAMCORE_EXPORT std::hash<String>
{
    NODISCARD SizeT operator() (const String &keyval) const noexcept
    {
        auto stringHash = hash<BaseString>();
        return stringHash(keyval);
    }
};

template <typename T>
struct IsStringUnqualified : std::false_type
{};
template <>
struct IsStringUnqualified<String> : std::true_type
{};
template <>
struct IsStringUnqualified<BaseString> : std::true_type
{};
template <>
struct IsStringUnqualified<StringView> : std::true_type
{};
template <>
struct IsStringUnqualified<TChar> : std::true_type
{};
// For char[] arrays
template <typename T, uint32 Num>
struct IsStringUnqualified<T[Num]> : IsStringUnqualified<std::remove_cvref_t<T>>
{};
// for char* pointers
template <typename T>
struct IsStringUnqualified<T *> : IsStringUnqualified<std::remove_cvref_t<T>>
{};

template <typename T>
struct IsString : IsStringUnqualified<std::remove_cvref_t<T>>
{};

template <typename... T>
struct IsStringTypes : std::conjunction<IsString<T>...>
{};

template <typename T>
concept StringType = IsString<T>::value;
template <typename T>
concept NonStringType = std::negation_v<IsString<T>>;
template <typename... T>
concept StringTypes = IsStringTypes<T...>::value;

#include "String/StringHelpers.inl"

#undef STRING_FUNCQUALIFIER