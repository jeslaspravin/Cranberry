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
    FORCE_INLINE CONST_EXPR String() : std::string() {}
    FORCE_INLINE CONST_EXPR String(const String& otherString) : std::string(otherString) {}
    FORCE_INLINE CONST_EXPR String(const String& otherString, size_type pos, size_type len) : std::string(otherString, pos, len) {}
    FORCE_INLINE CONST_EXPR String(const AChar* s, size_type n) : std::string(s, n) {}
    FORCE_INLINE CONST_EXPR String(const AChar* s) : std::string(s) {}
    FORCE_INLINE CONST_EXPR String(size_type n, AChar c) : std::string(n, c) {}
    FORCE_INLINE CONST_EXPR String(const std::string& otherString) : std::string(otherString) {}
    FORCE_INLINE CONST_EXPR String(std::string&& otherString) : std::string(otherString) {}
    FORCE_INLINE CONST_EXPR String(std::string::const_iterator start, std::string::const_iterator end) : std::string(start, end) {}

    FORCE_INLINE CONST_EXPR const AChar* getChar() const
    {
        return c_str();
    }

    FORCE_INLINE bool CONST_EXPR findAny(size_t& outIndex, String& outFoundString, const std::vector<String>& findStrgs, size_t offset = 0, bool fromEnd = false) const
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
        outFoundString = "";
        return false;
    }

    FORCE_INLINE CONST_EXPR String replaceAllCopy(const String& from, const String& to) const
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

    FORCE_INLINE CONST_EXPR void replaceAll(const String& from, const String& to)
    {
        size_t replaceAtPos = 0;
        while ((replaceAtPos = find(from, replaceAtPos)) != npos)
        {
            replace(replaceAtPos, from.length(), to);
            replaceAtPos += to.length();
        }
    }

    FORCE_INLINE CONST_EXPR bool startsWith(const String& match, bool bMatchCase = true) const
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

    FORCE_INLINE CONST_EXPR bool endsWith(const String& match, bool bMatchCase = true) const
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

    FORCE_INLINE CONST_EXPR void trimL()
    {
        erase(begin(), std::find_if(begin(), end(),
            [](unsigned char ch)
            {
                return !std::isspace(ch);
            })
        );
    }

    FORCE_INLINE CONST_EXPR void trimR()
    {
        erase(std::find_if(rbegin(), rend(),
            [](unsigned char ch)
            {
                return !std::isspace(ch);
            }).base()
                , end()
                );
    }

    FORCE_INLINE CONST_EXPR void trim()
    {
        trimL();
        trimR();
    }

    FORCE_INLINE CONST_EXPR String trimLCopy() const
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

    FORCE_INLINE CONST_EXPR String trimRCopy() const
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

    FORCE_INLINE CONST_EXPR String trimCopy() const
    {
        String s(*this);
        s.trim();
        return s;
    }

    template <typename IteratorType>
    CONST_EXPR static String join(IteratorType begin, IteratorType end, const String&& separator)
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

    CONST_EXPR static std::vector<String> split(const String& inStr, const String&& separator)
    {
        std::vector<String> outStrs;
        uint64 replaceAtPos = 0;
        uint64 offsetPos = 0;
        while ((replaceAtPos = inStr.find(separator, offsetPos)) != npos)
        {
            // Since offsetPos is end of last separator and foundAtPos is where this separator is found, Whatever in between is what we need
            outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cbegin() + replaceAtPos));
            offsetPos = replaceAtPos + separator.length();
        }
        // After final separator the suffix has to be added
        outStrs.emplace_back(String(inStr.cbegin() + offsetPos, inStr.cend()));
        return outStrs;
    }
};


template <>
struct PROGRAMCORE_EXPORT std::hash<String>
{
    NODISCARD size_t operator()(const String& keyval) const noexcept {
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
struct IsStringUnqualified<AChar> : std::true_type {};
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
    CONST_EXPR StringLiteral(const AChar (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    AChar value[N];
};