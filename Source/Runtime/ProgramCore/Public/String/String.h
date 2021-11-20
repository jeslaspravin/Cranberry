#pragma once

#include <string>
#include <type_traits>
#include <vector>
#include <algorithm>

#include "Types/CoreTypes.h"
#include "Types/CoreDefines.h"
#include "ProgramCoreExports.h"

#define STRINGIFY(...) #__VA_ARGS__

// No need to export as this is header only
class String : public std::string
{

public:
    FORCE_INLINE String() : std::string() {}
    FORCE_INLINE String(const String& otherString) : std::string(otherString) {}
    FORCE_INLINE String(const String& otherString, size_type pos, size_type len) : std::string(otherString, pos, len) {}
    FORCE_INLINE String(const AChar* s, size_type n) : std::string(s, n) {}
    FORCE_INLINE String(const AChar* s) : std::string(s) {}
    FORCE_INLINE String(size_type n, AChar c) : std::string(n, c) {}
    FORCE_INLINE String(const std::string& otherString) : std::string(otherString) {}
    FORCE_INLINE String(std::string&& otherString) : std::string(otherString) {}

    FORCE_INLINE const AChar* getChar() const
    {
        return c_str();
    }

    FORCE_INLINE bool findAny(size_t& outIndex, String& outFoundString, const std::vector<String>& findStrgs, size_t offset = 0, bool fromEnd = false) const
    {
        size_t foundAt = npos;
        for (const String& strg : findStrgs)
        {
            size_t foundAtNew = fromEnd ? rfind(strg, length() - offset) : find(strg, offset);
            if (foundAtNew != npos) {
                outIndex = (foundAt == npos || (fromEnd ? foundAt<foundAtNew : foundAt>foundAtNew)) ? foundAtNew : foundAt;
                outFoundString = (foundAt == npos || (fromEnd ? foundAt<foundAtNew : foundAt>foundAtNew)) ? strg : outFoundString;
                foundAt = outIndex;
            }
        }
        if (foundAt != npos)
        {
            return true;
        }
        outIndex = npos;
        outFoundString = "";
        return false;
    }

    FORCE_INLINE String replaceAllCopy(const String& from, const String& to) const
    {
        String newStr = this->getChar();
        size_t replaceAtPos = 0;
        while ((replaceAtPos = newStr.find(from, replaceAtPos)) != npos)
        {
            newStr.replace(replaceAtPos, from.size(), to);
            replaceAtPos += to.size();
        }
        return newStr;
    }

    FORCE_INLINE void replaceAll(const String& from, const String& to)
    {
        size_t replaceAtPos = 0;
        while ((replaceAtPos = find(from, replaceAtPos)) != npos)
        {
            replace(replaceAtPos, from.size(), to);
            replaceAtPos += to.size();
        }
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

    FORCE_INLINE void trimL()
    {
        erase(begin(), std::find_if(begin(), end(),
            [](unsigned char ch)
            {
                return !std::isspace(ch);
            })
        );
    }

    FORCE_INLINE void trimR()
    {
        erase(std::find_if(rbegin(), rend(),
            [](unsigned char ch)
            {
                return !std::isspace(ch);
            }).base()
                , end()
                );
    }

    FORCE_INLINE void trim()
    {
        trimL();
        trimR();
    }

    FORCE_INLINE String trimLCopy() const
    {
        String s(*this);
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            [](unsigned char ch)
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
            [](unsigned char ch)
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

};


template <>
struct PROGRAMCORE_EXPORT std::hash<String>
{

    _NODISCARD size_t operator()(const String& keyval) const noexcept {
        auto stringHash = hash<std::string>();
        return stringHash(keyval);
    }
};

template<typename T>
struct IsStringUnqualified : std::false_type {};
template<>
struct IsStringUnqualified<String> : std::true_type {};
template<>
struct IsStringUnqualified<std::string> : std::true_type {};
template<>
struct IsStringUnqualified<AChar*> : std::true_type {};
template<>
struct IsStringUnqualified<const AChar*> : std::true_type {};

template<typename T>
struct IsString : IsStringUnqualified<typename std::remove_cv<typename std::remove_reference<T>::type>::type> {};

template <typename... T>
struct IsStringTypes : std::conjunction<IsString<T>...> {};