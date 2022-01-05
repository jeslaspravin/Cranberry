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
    //template<size_t N>
    //CONST_EXPR String(const AChar(&str)[N]) : std::string(str) {}
    FORCE_INLINE String(const std::string& otherString) : std::string(otherString) {}
    FORCE_INLINE String(std::string&& otherString) : std::string(otherString) {}
    FORCE_INLINE String(std::string::const_iterator start, std::string::const_iterator end) : std::string(start, end) {}
    FORCE_INLINE String(const std::string_view& strView) : std::string(strView) {}

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
            [](unsigned char ch)
            {
                return !std::isspace(ch);
            })
        );
        return *this;
    }

    FORCE_INLINE String& trimR()
    {
        erase(std::find_if(rbegin(), rend(),
            [](const AChar& ch)
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

    /*
    * Splits given string into list of line views
    */
    std::vector<std::string_view> splitLines() const
    {
        std::vector<std::string_view> outStrs;
        uint64 foundAtPos = 0;
        uint64 offsetPos = 0;
        while ((foundAtPos = find('\n', offsetPos)) != npos)
        {
            // If previous char is valid and it is carriage return(\r) then we have to consider that as part of CR-LF
            if (foundAtPos != 0 || cbegin()[foundAtPos - 1] == '\r')
            {
                outStrs.emplace_back(std::string_view(cbegin() + offsetPos, cbegin() + (foundAtPos - 1)));
            }
            else
            {
                // Since offsetPos is end of last separator and foundAtPos is where this separator is found, Whatever in between is what we need
                outStrs.emplace_back(std::string_view(cbegin() + offsetPos, cbegin() + foundAtPos));
            }
            // Post CR-LF char
            offsetPos = foundAtPos + 1;
        }
        // After final separator the suffix has to be added if there is any char after final CR-LF
        if ((cbegin() + offsetPos) != cend())
        {
            outStrs.emplace_back(std::string_view(cbegin() + offsetPos, cend()));
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

template <StringLiteral StoreValue>
struct StringLiteralStore
{
    using LiteralType = decltype(StoreValue);
    CONST_EXPR static const LiteralType Literal = StoreValue;

    CONST_EXPR const AChar* getChar() const
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