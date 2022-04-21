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
        struct
        {
            uint8 b3;
            uint8 b2;
            uint8 b1;
            uint8 b0;
        };
        struct
        {
            uint16 highWord;
            uint16 lowWord;
        };
#else  // BIG_ENDIAN
        struct
        {
            uint8 b0;
            uint8 b1;
            uint8 b2;
            uint8 b3;
        };
        struct
        {
            uint16 lowWord;
            uint16 highWord;
        };
#endif // BIG_ENDIAN
        uint32 dw;
    };
    // Actual data union
    union
    {
        uint32 components[4];
        struct
        {
            Component _a;
            Component _b;
            Component _c;
            Component _d;
        };
        struct
        {
            uint32 a;
            uint32 b;
            uint32 c;
            uint32 d;
        };
    };

public:
    CONST_EXPR CBEGuid()
        : a(0)
        , b(0)
        , c(0)
        , d(0)
    {}
    // Generates Guid to valid value
    CBEGuid(EInitType);
    CONST_EXPR CBEGuid(uint32 inA, uint32 inB, uint32 inC, uint32 inD)
        : a(inA)
        , b(inB)
        , c(inC)
        , d(inD)
    {}

    // Equality
    CONST_EXPR std::strong_ordering operator<=>(const CBEGuid &rhs) const noexcept
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
    CONST_EXPR bool operator==(const CBEGuid &rhs) const noexcept { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
    FORCE_INLINE bool isValid() const { return (a | b | c | d) > 0; };

    String toString(EGuidFormat format = DWordWithHyphen) const;

    // Static functions
    NODISCARD FORCE_INLINE static CBEGuid create() { return CBEGuid(EInitType::InitType_ForceInit); }
    // Parsing from string functions both string and const_expr variant
    NODISCARD static CBEGuid parse(const String &str);
    NODISCARD static CBEGuid parseFormat(const String &str, EGuidFormat format);
    template <typename CharType>
    NODISCARD CONST_EXPR static CBEGuid parse(const CharType *str, SizeT len)
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
                TCharStr::startsWith(str, '{', true) ? ParseFromFormat<DigitsInBraces, CharType>{}(str, len)
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
    NODISCARD CONST_EXPR static CBEGuid parse(const CharType *str)
    {
        return parse(str, TCharStr::length(str));
    }
    template <typename CharType>
    NODISCARD CONST_EXPR static CBEGuid parseFormat(const CharType *str, SizeT len, EGuidFormat format)
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
    NODISCARD CONST_EXPR static CBEGuid parseFormat(const CharType *str, EGuidFormat format)
    {
        return parseFormat(str, TCharStr::length(str), format);
    }
};

template <>
struct PROGRAMCORE_EXPORT std::hash<CBEGuid>
{
    NODISCARD SizeT operator()(const CBEGuid &val) const noexcept
    {
        SizeT hashVal = 0;
        HashUtility::hashAllInto(hashVal, val.a, val.b, val.c, val.d);
        return hashVal;
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsOnly, CharType>
{
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD
        debugAssert(len == 32);
        uint32 a, b, c, d;
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
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // AAAAAAAA-BBBBBBBB-CCCCCCCC-DDDDDDDD
        debugAssert(len == 35);
        uint32 a, b, c, d;
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
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // {0xAAAAAAAA,0xBBBB,0xBBBB,{0xCC,0xCC,0xCC,0xCC,0xDD,0xDD,0xDD,0xDD}}
        debugAssert(len == 68);
        Component a, b, c, d;
        bool bParsed = TCharUtils::parseHex(a.dw, CharStringView<CharType>(str + 1, 10));
        bParsed = bParsed && TCharUtils::parseHex(b.highWord, CharStringView<CharType>(str + 12, 6));
        bParsed = bParsed && TCharUtils::parseHex(b.lowWord, CharStringView<CharType>(str + 19, 6));
        bParsed = bParsed && TCharUtils::parseHex(c.b3, CharStringView<CharType>(str + 27, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.b2, CharStringView<CharType>(str + 32, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.b1, CharStringView<CharType>(str + 37, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.b0, CharStringView<CharType>(str + 42, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.b3, CharStringView<CharType>(str + 47, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.b2, CharStringView<CharType>(str + 52, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.b1, CharStringView<CharType>(str + 57, 4));
        bParsed = bParsed && TCharUtils::parseHex(d.b0, CharStringView<CharType>(str + 62, 4));

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
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD
        debugAssert(len == 36);
        Component a, b, c, d;
        bool bParsed = TCharUtils::parseHex(a.dw, CharStringView<CharType>(str, 8));
        bParsed = bParsed && TCharUtils::parseHex(b.highWord, CharStringView<CharType>(str + 9, 4));
        bParsed = bParsed && TCharUtils::parseHex(b.lowWord, CharStringView<CharType>(str + 14, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.highWord, CharStringView<CharType>(str + 19, 4));
        bParsed = bParsed && TCharUtils::parseHex(c.lowWord, CharStringView<CharType>(str + 24, 4));
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
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // {AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD}
        debugAssert(len == 38);
        return CBEGuid::ParseFromFormat<CBEGuid::DigitsWithHyphen, CharType>{}(str + 1, len - 2);
    }
};

template <typename CharType>
struct CBEGuid::ParseFromFormat<CBEGuid::DigitsInParans, CharType>
{
    NODISCARD CONST_EXPR CBEGuid operator()(const CharType *str, SizeT len) const
    {
        // (AAAAAAAA-BBBB-BBBB-CCCC-CCCCDDDDDDDD)
        debugAssert(len == 38);
        return CBEGuid::ParseFromFormat<CBEGuid::DigitsWithHyphen, CharType>{}(str + 1, len - 2);
    }
};