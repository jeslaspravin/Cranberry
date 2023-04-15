/*!
 * \file Guid.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "String/TCharString.h"
#include "Types/HashTypes.h"

//
// Specification https://www.ietf.org/rfc/rfc4122.txt
struct PROGRAMCORE_EXPORT CBEGuid
{
public:
    enum EGuidFormat
    {
        DigitsOnly,       // AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD
        HexValues,        // {0xAAAAAAAA,0xBBBB,0xBBBB,{0xCC,0xCC,0xCC,0xCC,0xDD,0xDD,0xDD,0xDD}}
        DigitsWithHyphen, // AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD
        DigitsInBraces,   // {AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD}
        DigitsInParans,   // (AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD)
        DWordWithHyphen   // AAAAAAAA-BBBBBBBB-CCCCCCCC-DDDDDDDD
    };
    template <EGuidFormat Format, typename CharType>
    struct ParseFromFormat;

    // Component views
    union Component
    {
#if BIG_ENDIAN
        struct CompBytes
        {
            uint8 b3;
            uint8 b2;
            uint8 b1;
            uint8 b0;
        } bytes;
        struct CompWords
        {
            uint16 highWord;
            uint16 lowWord;
        } words;
#else  // BIG_ENDIAN
        struct CompBytes
        {
            uint8 b0;
            uint8 b1;
            uint8 b2;
            uint8 b3;
        } bytes;
        struct CompWords
        {
            uint16 lowWord;
            uint16 highWord;
        } words;
#endif // BIG_ENDIAN
        uint32 dw;
    };
    // Actual data union
    union
    {
        uint32 components[4];

        struct IDComponents
        {
            Component a;
            Component b;
            Component c;
            Component d;
        } _comps;

        struct IDParts
        {
            uint32 a;
            uint32 b;
            uint32 c;
            uint32 d;
        } parts;
    };

public:
    constexpr CBEGuid()
        : parts{ 0, 0, 0, 0 }
    {}
    // Generates Guid to valid value
    CBEGuid(EInitType);
    constexpr CBEGuid(uint32 inA, uint32 inB, uint32 inC, uint32 inD)
        : parts{ inA, inB, inC, inD }
    {}

    // Equality
    constexpr std::strong_ordering operator<=> (const CBEGuid &rhs) const noexcept
    {
        for (int32 i = 0; i < ARRAY_LENGTH(components); ++i)
        {
            if (components[i] != rhs.components[i])
            {
                return components[i] < rhs.components[i] ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
        return std::strong_ordering::equal;
    }
    // Better than xor based equality
    constexpr bool operator== (const CBEGuid &rhs) const noexcept
    {
        return parts.a == rhs.parts.a && parts.b == rhs.parts.b && parts.c == rhs.parts.c && parts.d == rhs.parts.d;
    }
    FORCE_INLINE bool isValid() const { return (parts.a | parts.b | parts.c | parts.d) > 0; }

    String toString(EGuidFormat format = DWordWithHyphen) const;

    // Static functions
    NODISCARD FORCE_INLINE static CBEGuid create() { return CBEGuid(EInitType::InitType_ForceInit); }
    // Parsing from string functions both string and const_expr variant
    NODISCARD static CBEGuid parse(const String &str);
    NODISCARD static CBEGuid parseFormat(const String &str, EGuidFormat format);
    template <typename CharType>
    NODISCARD constexpr static CBEGuid parse(const CharType *str, SizeT len)
    {
        switch (len)
        {
        case 32:
            return ParseFromFormat<DigitsOnly, CharType>{}(str, len);
        case 35:
            return ParseFromFormat<DWordWithHyphen, CharType>{}(str, len);
        case 36:
            return ParseFromFormat<DigitsWithHyphen, CharType>{}(str, len);
        case 38:
            return (
                TCharStr::startsWith<CharType>(str, '{', true) ? ParseFromFormat<DigitsInBraces, CharType>{}(str, len)
                                                               : ParseFromFormat<DigitsInParans, CharType>{}(str, len)
            );
        case 68:
            return ParseFromFormat<HexValues, CharType>{}(str, len);
        default:
            break;
        }
        return {};
    }
    template <typename CharType>
    NODISCARD constexpr static CBEGuid parse(const CharType *str)
    {
        return parse(str, TCharStr::length(str));
    }
    template <typename CharType>
    NODISCARD constexpr static CBEGuid parseFormat(const CharType *str, SizeT len, EGuidFormat format)
    {
        switch (format)
        {
        case CBEGuid::DigitsOnly:
            return ParseFromFormat<DigitsOnly, CharType>{}(str, len);
        case CBEGuid::HexValues:
            return ParseFromFormat<HexValues, CharType>{}(str, len);
        case CBEGuid::DigitsWithHyphen:
            return ParseFromFormat<DigitsWithHyphen, CharType>{}(str, len);
        case CBEGuid::DigitsInBraces:
            return ParseFromFormat<DigitsInBraces, CharType>{}(str, len);
        case CBEGuid::DigitsInParans:
            return ParseFromFormat<DigitsInParans, CharType>{}(str, len);
        case CBEGuid::DWordWithHyphen:
            return ParseFromFormat<DWordWithHyphen, CharType>{}(str, len);
        default:
            break;
        }
        return {};
    }
    template <typename CharType>
    NODISCARD constexpr static CBEGuid parseFormat(const CharType *str, EGuidFormat format)
    {
        return parseFormat(str, TCharStr::length(str), format);
    }
};

template <>
struct PROGRAMCORE_EXPORT std::hash<CBEGuid>
{
    NODISCARD SizeT operator() (const CBEGuid &val) const noexcept
    {
        SizeT hashVal = 0;
        HashUtility::hashAllInto(hashVal, val.parts.a, val.parts.b, val.parts.c, val.parts.d);
        return hashVal;
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsOnly, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD
        if (len != 32)
        {
            return {};
        }

        uint32 a = 0, b = 0, c = 0, d = 0;
        bool bParsed = TCharUtils::parseHex(a, CharStringView<CharType>(str, 8));
        bParsed = bParsed && TCharUtils::parseHex(b, CharStringView<CharType>(str + 8, 8));
        bParsed = bParsed && TCharUtils::parseHex(c, CharStringView<CharType>(str + 16, 8));
        bParsed = bParsed && TCharUtils::parseHex(d, CharStringView<CharType>(str + 24, 8));

        if (bParsed)
        {
            return CBEGuid(a, b, c, d);
        }
        return {};
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DWordWithHyphen, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // AAAAAAAA-BBBBBBBB-CCCCCCCC-DDDDDDDD
        if (len != 35)
        {
            return {};
        }

        uint32 a = 0, b = 0, c = 0, d = 0;
        bool bParsed = TCharUtils::parseHex(a, CharStringView<CharType>(str, 8));
        bParsed = bParsed && TCharUtils::parseHex(b, CharStringView<CharType>(str + 9, 8));
        bParsed = bParsed && TCharUtils::parseHex(c, CharStringView<CharType>(str + 18, 8));
        bParsed = bParsed && TCharUtils::parseHex(d, CharStringView<CharType>(str + 27, 8));

        if (bParsed)
        {
            return CBEGuid(a, b, c, d);
        }
        return {};
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::HexValues, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // {0xAAAAAAAA,0xBBBB,0xBBBB,{0xCC,0xCC,0xCC,0xCC,0xDD,0xDD,0xDD,0xDD}}
        if (len != 68)
        {
            return {};
        }

        Component a = {}, b = {}, c = {}, d = {};
        bool bParsed = TCharUtils::parseHex(a.dw, CharStringView<CharType>(str + 1, 10));
        bParsed = bParsed && TCharUtils::parseHex(b.words.highWord, CharStringView<CharType>(str + 12, 6));
        bParsed = bParsed && TCharUtils::parseHex(b.words.lowWord, CharStringView<CharType>(str + 19, 6));
        bParsed = bParsed && TCharUtils::parseHex(c.bytes.b3, CharStringView<CharType>(str + 27, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.bytes.b2, CharStringView<CharType>(str + 32, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.bytes.b1, CharStringView<CharType>(str + 37, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.bytes.b0, CharStringView<CharType>(str + 42, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.bytes.b3, CharStringView<CharType>(str + 47, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.bytes.b2, CharStringView<CharType>(str + 52, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.bytes.b1, CharStringView<CharType>(str + 57, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.bytes.b0, CharStringView<CharType>(str + 62, 4));

        if (bParsed)
        {
            return CBEGuid(a.dw, b.dw, c.dw, d.dw);
        }
        return {};
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsWithHyphen, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD
        if (len != 36)
        {
            return {};
        }

        Component a = {}, b = {}, c = {}, d = {};
        bool bParsed = TCharUtils::parseHex(a.dw, CharStringView<CharType>(str, 8));
        bParsed = bParsed && TCharUtils::parseHex(b.words.highWord, CharStringView<CharType>(str + 9, 4));
        bParsed = bParsed && TCharUtils::parseHex(b.words.lowWord, CharStringView<CharType>(str + 14, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.words.highWord, CharStringView<CharType>(str + 19, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.words.lowWord, CharStringView<CharType>(str + 24, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.dw, CharStringView<CharType>(str + 28, 8));

        if (bParsed)
        {
            return CBEGuid(a.dw, b.dw, c.dw, d.dw);
        }
        return {};
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsInBraces, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // {AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD}
        if (len != 38)
        {
            return {};
        }
        return CBEGuid::ParseFromFormat<CBEGuid::DigitsWithHyphen, CharType>{}(str + 1, len - 2);
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsInParans, CharType>
{
    NODISCARD constexpr CBEGuid operator() (const CharType *str, SizeT len) const
    {
        // (AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD)
        if (len != 38)
        {
            return {};
        }
        return CBEGuid::ParseFromFormat<CBEGuid::DigitsWithHyphen, CharType>{}(str + 1, len - 2);
    }
};