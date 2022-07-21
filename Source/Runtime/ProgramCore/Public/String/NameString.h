/*!
 * \file NameString.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String.h"
#include "StringID.h"

// Since STRINGID is used inside NameString we start with STRINGID constexpr check
#if HAS_STRINGID_CONSTEXPR
#if HAS_STRING_CONSTEXPR
#define STRING_FUNCQUALIFIER CONST_EXPR
#else // HAS_STRING_CONSTEXPR
#define STRING_FUNCQUALIFIER FORCE_INLINE
#endif // HAS_STRING_CONSTEXPR
#else  // HAS_STRINGID_CONSTEXPR
#define STRING_FUNCQUALIFIER FORCE_INLINE
#endif // HAS_STRINGID_CONSTEXPR

/*!
 * \class NameString
 *
 * \brief Class combines both String ID and String.
 * This is useful in case where we need both string for logic or pretty display purposes and need id for fast comparison or hashing
 *
 * \author Jeslas
 * \date June 2022
 */
class NameString
{
private:
    String nameStr;
    StringID id;

    template <ArchiveTypeName ArchiveType>
    friend ArchiveType &operator<<(ArchiveType &archive, NameString &value);

public:
    STRING_FUNCQUALIFIER NameString() = default;
    STRING_FUNCQUALIFIER NameString(const NameString &) = default;
    STRING_FUNCQUALIFIER NameString(NameString &&) = default;
    STRING_FUNCQUALIFIER NameString &operator=(const NameString &) = default;
    STRING_FUNCQUALIFIER NameString &operator=(NameString &&) = default;

    STRING_FUNCQUALIFIER explicit NameString(EInitType initType)
        : nameStr()
        , id(initType)
    {}
    // Additional constructors
    FORCE_INLINE explicit NameString(const String &str)
        : nameStr(str)
        , id(nameStr)
    {}
    STRING_FUNCQUALIFIER
    NameString(const AChar *str, SizeT len)
        : nameStr(str, len)
        , id(str, len)
    {}
    STRING_FUNCQUALIFIER
    NameString(const AChar *str)
        : nameStr(UTF8_TO_TCHAR(str))
        , id(str)
    {}
    STRING_FUNCQUALIFIER
    NameString(const WChar *str)
        : nameStr(WCHAR_TO_TCHAR(str))
        , id(str)
    {}
    // Additional assignments
    FORCE_INLINE NameString &operator=(const String &str)
    {
        nameStr = str;
        id = nameStr;
        return *this;
    }
    STRING_FUNCQUALIFIER NameString &operator=(const AChar *str)
    {
        nameStr = UTF8_TO_TCHAR(str);
        id = str;
        return *this;
    }
    STRING_FUNCQUALIFIER NameString &operator=(const WChar *str)
    {
        nameStr = WCHAR_TO_TCHAR(str);
        id = str;
        return *this;
    }

    // Equality
    CONST_EXPR std::strong_ordering operator<=>(const NameString &rhs) const noexcept { return id <=> rhs.id; }
    // Since the == operator is necessary even though spaceship is defined for hash map
    CONST_EXPR bool operator==(const NameString &rhs) const noexcept { return id == rhs.id; }
    // String ID equality
    friend CONST_EXPR std::strong_ordering operator<=>(const NameString &lhs, const StringID &rhs) noexcept { return lhs.id <=> rhs; }
    friend CONST_EXPR bool operator==(const NameString &lhs, const StringID &rhs) noexcept { return lhs.id == rhs; }
    friend CONST_EXPR std::strong_ordering operator<=>(const StringID &lhs, const NameString &rhs) noexcept { return lhs <=> rhs.id; }
    friend CONST_EXPR bool operator==(const StringID &lhs, const NameString &rhs) noexcept { return lhs == rhs.id; }

    FORCE_INLINE const String &toString() const { return nameStr; }
    FORCE_INLINE bool isValid() const noexcept { return id.isValid(); }
    FORCE_INLINE explicit operator StringID() const noexcept { return id; }
    FORCE_INLINE explicit operator String() const noexcept { return nameStr; }
};

template <>
struct PROGRAMCORE_EXPORT std::hash<NameString>
{
    NODISCARD SizeT operator()(const NameString &keyval) const noexcept { return StringID(keyval).getID(); }
};

#undef STRING_FUNCQUALIFIER