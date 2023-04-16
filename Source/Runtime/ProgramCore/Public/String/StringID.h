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

#include "Serialization/ArchiveTypes.h"
#include "Types/CompilerDefines.h"
#include "Types/xxHash/xxHashInclude.hpp"

#if DEV_BUILD
#define STRINGID_FUNCQUALIFIER FORCE_INLINE
#define STRINGID_CONSTEXPR
#define HAS_STRINGID_CONSTEXPR 0
#define ENABLE_STRID_DEBUG 1
#else // DEV_BUILD
#define STRINGID_FUNCQUALIFIER constexpr
#define STRINGID_CONSTEXPR constexpr
#define HAS_STRINGID_CONSTEXPR 1
#define ENABLE_STRID_DEBUG 0
#endif // DEV_BUILD

#ifndef STRINGID_HASHFUNC
#define STRINGID_HASHFUNC xxHash::hashString
#endif // !STRINGID_HASHFUNC

#if ENABLE_STRID_DEBUG
#include <unordered_map>
#include <unordered_set>
#endif

class StringID;
struct DebugStringIDsData;

inline namespace Literals
{
NODISCARD STRINGID_FUNCQUALIFIER StringID operator"" _sid (const TChar *str, SizeT len) noexcept;
}

class PROGRAMCORE_EXPORT StringID
{
public:
    using IDType = uint32;

private:
    IDType id = 0;

    friend DebugStringIDsData;

    friend STRINGID_FUNCQUALIFIER StringID Literals::operator"" _sid (const TChar *str, SizeT len) noexcept;
    template <ArchiveTypeName ArchiveType>
    friend ArchiveType &operator<< (ArchiveType &archive, StringID &value);

    CONST_INIT static const IDType Seed = STRINGID_HASHFUNC(TCHAR("Cranberry_StringID"), IDType(0));

public:
    static const StringID INVALID;

private:
#if ENABLE_STRID_DEBUG
    using DebugStringsMap = std::unordered_map<IDType, std::unordered_set<String>>;

    // Holds pointer to debugStringsDB which will be used by debug to visualize string
    static DebugStringsMap *debugStrings;
    static const TChar *findDebugString(IDType strId);
    void insertDbgStr(StringView str);

    StringID(IDType strId, const TChar *debugStr, SizeT len)
        : id(strId)
    {
        insertDbgStr({ debugStr, len });
    }
    explicit StringID(IDType strId)
        : id(strId)
    {
        const TChar *debugStr = findDebugString(id);
        CompilerHacks::ignoreUnused(debugStr);
    }
#else
    constexpr StringID(IDType strId)
        : id(strId)
    {}

    constexpr void insertDbgStr(StringView) {}
#endif

    STRINGID_FUNCQUALIFIER void initFromAChar(const AChar *str)
    {
        id = STRINGID_HASHFUNC(str, Seed);
        insertDbgStr(UTF8_TO_TCHAR(str));
    }

public:
    constexpr StringID() = default;
    constexpr StringID(const StringID &) = default;
    constexpr StringID(StringID &&) = default;
    constexpr StringID &operator= (const StringID &) = default;
    constexpr StringID &operator= (StringID &&) = default;

    constexpr explicit StringID(EInitType)
        : id(0)
    {}
    // Additional constructors
    FORCE_INLINE explicit StringID(StringView str)
        : id(STRINGID_HASHFUNC(str, Seed))
    {
        insertDbgStr(str);
    }
    STRINGID_FUNCQUALIFIER
    StringID(const AChar *str, SizeT len)
        : id(STRINGID_HASHFUNC(str, IDType(len), Seed))
    {
        insertDbgStr({ str, len });
    }
    STRINGID_FUNCQUALIFIER
    StringID(const AChar *str) { initFromAChar(str); }
    STRINGID_FUNCQUALIFIER
    StringID(const WChar *str) { initFromAChar(TCHAR_TO_UTF8(WCHAR_TO_TCHAR(str))); }
    // Additional assignments
    FORCE_INLINE
    StringID &operator= (StringView str)
    {
        id = STRINGID_HASHFUNC(str, Seed);
        insertDbgStr(str);
        return *this;
    }
    STRINGID_FUNCQUALIFIER
    StringID &operator= (const AChar *str)
    {
        initFromAChar(str);
        return *this;
    }
    STRINGID_FUNCQUALIFIER
    StringID &operator= (const WChar *str)
    {
        initFromAChar(TCHAR_TO_UTF8(WCHAR_TO_TCHAR(str)));
        return *this;
    }
    // Equality
    constexpr std::strong_ordering operator<=> (const StringID &rhs) const noexcept { return id <=> rhs.id; }
    // Since the == operator is necessary even though spaceship is defined for hash map
    constexpr bool operator== (const StringID &rhs) const noexcept { return id == rhs.id; }

    /**
     * toString will return id integer as string in release build
     * NOTE : Do not use this toString for any purpose other than debug logging
     * Use NameString if you need toString() for logic
     */
    FORCE_INLINE String toString() const
    {
#if ENABLE_STRID_DEBUG
        if (const TChar *foundStr = findDebugString(id))
        {
            return foundStr;
        }
#endif // DEV_BUILD
        return String::toString(id);
    }

    FORCE_INLINE IDType getID() const noexcept { return id; }
    FORCE_INLINE bool isValid() const noexcept { return id != 0; }
    FORCE_INLINE explicit operator IDType () const noexcept { return id; }
};

#define STRID(CharStr) COMBINE(TCHAR(CharStr), _sid)

inline namespace Literals
{
NODISCARD STRINGID_FUNCQUALIFIER StringID operator"" _sid (const TChar *str, SizeT len) noexcept
{
#if ENABLE_STRID_DEBUG
    return StringID(STRINGID_HASHFUNC(str, StringID::IDType(len), StringID::Seed), str, len);
#else  // DEV_BUILD
    return StringID(STRINGID_HASHFUNC(str, StringID::IDType(len), StringID::Seed));
#endif // DEV_BUILD
}
} // namespace Literals

template <>
struct PROGRAMCORE_EXPORT std::hash<StringID>
{
    NODISCARD SizeT operator() (const StringID &keyval) const noexcept { return keyval.getID(); }
};

#undef STRINGID_FUNCQUALIFIER