/*!
 * \file StringID.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/xxHash/xxHashInclude.hpp"

#if DEV_BUILD
#define STRINGID_FUNCQUALIFIER FORCE_INLINE
#else // DEV_BUILD
#define STRINGID_FUNCQUALIFIER CONST_EXPR
#endif // DEV_BUILD

#ifndef STRINGID_HASHFUNC
#define STRINGID_HASHFUNC xxHash::hashString
#endif // !STRINGID_HASHFUNC


class StringID;
inline namespace Literals
{
    NODISCARD STRINGID_FUNCQUALIFIER StringID operator"" _sid(const TChar * str, SizeT len) noexcept;
}

class PROGRAMCORE_EXPORT StringID
{
public:
    using IDType = uint32;
private:
    IDType id;

    friend STRINGID_FUNCQUALIFIER StringID Literals::operator"" _sid(const TChar * str, SizeT len) noexcept;

    CONST_INIT static const IDType Seed = STRINGID_HASHFUNC(TCHAR("Cranberry_StringID"), IDType(0));
private:

#if DEV_BUILD
    // Holds pointer to debugStringsDB which will be used by debug to visualize string
    const std::unordered_map<StringID::IDType, String>* debugStrings = nullptr;
    static std::unordered_map<IDType, String>& debugStringDB();
    template <typename StrType>
    void insertDbgStr(StrType&& str)
    {
        debugStringDB().insert({ id, std::forward<StrType>(str) });
        debugStrings = &debugStringDB();
    }

    StringID(IDType strId, const TChar* debugStr, SizeT len)
        : id(strId)
    {
        insertDbgStr(String(debugStr, len));
    }
#else
    CONST_EXPR StringID(IDType strId)
        : id(strId)
    {}

    template <typename StrType>
    void insertDbgStr(StrType&&){}
#endif

    STRINGID_FUNCQUALIFIER void initFromAChar(const AChar* str)
    {
        id = STRINGID_HASHFUNC(str, Seed);
        insertDbgStr(String(UTF8_TO_TCHAR(str)));
    }
public:
    CONST_EXPR StringID() = default;
    CONST_EXPR StringID(const StringID&) = default;
    CONST_EXPR StringID(StringID&&) = default;
    CONST_EXPR StringID& operator=(const StringID&) = default;
    CONST_EXPR StringID& operator=(StringID&&) = default;

    CONST_EXPR explicit StringID(EInitType) : id(0) {}
    // Additional constructors
    FORCE_INLINE StringID(const String& str)
        : id(STRINGID_HASHFUNC(str, Seed))
    {
        insertDbgStr(str);
    }
    STRINGID_FUNCQUALIFIER
    StringID(const AChar* str, SizeT len)
        : id(STRINGID_HASHFUNC(str, IDType(len) , Seed))
    {
        insertDbgStr(String(str, len));
    }
    STRINGID_FUNCQUALIFIER
    StringID(const AChar* str)
    {
        initFromAChar(str);
    }
    STRINGID_FUNCQUALIFIER
    StringID(const WChar* str)
    {
        initFromAChar(TCHAR_TO_UTF8(WCHAR_TO_TCHAR(str)));
    }
    // Additional assignments
    FORCE_INLINE StringID& operator=(const String& str)
    {
        id = STRINGID_HASHFUNC(str, Seed);
        insertDbgStr(str);
        return *this;
    }
    STRINGID_FUNCQUALIFIER StringID& operator=(const AChar* str)
    {
        initFromAChar(str);
        return *this;
    }
    STRINGID_FUNCQUALIFIER StringID& operator=(const WChar* str)
    {
        initFromAChar(TCHAR_TO_UTF8(WCHAR_TO_TCHAR(str)));
        return *this;
    }
    // Equality
    CONST_EXPR std::strong_ordering operator<=>(const StringID& rhs) const noexcept
    {
        return id <=> rhs.id;
    }
    // Since the == operator is necessary even though spaceship is defined for hash map
    CONST_EXPR bool operator==(const StringID& rhs) const noexcept
    {
        return id == rhs.id;
    }

    /**
     * toString will return id integer as string in release build
     * NOTE : Do not use this toString for any purpose other than debug logging
     */
    FORCE_INLINE String toString() const
    {
#if DEV_BUILD
        return debugStringDB().find(id)->second;
#else // DEV_BUILD
        return String::toString(id);
#endif // DEV_BUILD
    }

    FORCE_INLINE IDType getID() const noexcept
    {
        return id;
    }
    FORCE_INLINE bool isValid() const noexcept
    {
        return id != 0;
    }
    FORCE_INLINE explicit operator IDType() const noexcept
    {
        return id;
    }
};

#define STRID(CharStr) TCHAR(CharStr)_sid

inline namespace Literals
{
    NODISCARD STRINGID_FUNCQUALIFIER StringID operator"" _sid(const TChar * str, SizeT len) noexcept
    {
#if DEV_BUILD
        return StringID(STRINGID_HASHFUNC(str, StringID::IDType(len), StringID::Seed), str, len);
#else // DEV_BUILD
        return StringID(STRINGID_HASHFUNC(str, StringID::IDType(len), StringID::Seed));
#endif // DEV_BUILD
    }
}

template <>
struct PROGRAMCORE_EXPORT std::hash<StringID>
{
    NODISCARD SizeT operator()(const StringID& keyval) const noexcept 
    {
        return keyval.getID();
    }
};

#undef STRINGID_FUNCQUALIFIER