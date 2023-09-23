/*!
 * \file StringHelpers.inl
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

template <typename CharType>
struct ToStringImpl
{
    using value_type = CharType;

    template <typename Type>
    static String toString(Type &&value) noexcept
    {
        static_assert(DependentFalseTypeValue<Type>, "Unsupported CharType to convert into");
        return {};
    }
};

struct StringCodePointsHelper
{
    static uint64 count(const AChar *startChar) noexcept;
    static uint64 count(const WChar *startChar) noexcept;

    PROGRAMCORE_EXPORT static void validateStartCode(AChar startChar) noexcept;
    PROGRAMCORE_EXPORT static void validateStartCode(WChar startChar) noexcept;
    FORCE_INLINE static uint32 utf8ToCode(const Utf8 *firstChar, uint32 byteCount) noexcept;
    FORCE_INLINE static uint32 utf16ToCode(const Utf16 *firstChar) noexcept;
    // startChar must be valid a code point start
    FORCE_INLINE static uint32 codePoint(AChar const *&endChar, AChar const *&startChar) noexcept;
    FORCE_INLINE static uint32 codePoint(WChar const *&endChar, WChar const *&startChar) noexcept;

    // Gets next/previous code point from given character start and reference outs endChar for this
    // variable length character
    FORCE_INLINE static uint32 nextCodePoint(AChar const *&endChar, AChar const *&startChar) noexcept;
    FORCE_INLINE static uint32 nextCodePoint(WChar const *&endChar, WChar const *&startChar) noexcept;
    // Previous must be invoked from 1st char and never from 0th char
    FORCE_INLINE static uint32 prevCodePoint(AChar const *&endChar, AChar const *&startChar) noexcept;
    FORCE_INLINE static uint32 prevCodePoint(WChar const *&endChar, WChar const *&startChar) noexcept;
};

// Remember we are only converting to UTF-8 when we convert to AChar or char or char8_t
// For now it is okay if ASCII/ANSI end up as garbage
template <typename FromCharType, typename ToCharType>
struct StringConv
{
    // End is inclusive of null-terminated character
    const ToCharType *convert(const FromCharType *start) noexcept
    {
        static_assert(DependentFalseTypeValue<StringConv>, "Unsupported CharType conversion combination");
        return nullptr;
    }
};

// Converter that uses STL to convert between an encoded format(UTF-16/UTF-32) and UTF-8,
// This is not for converting between platform string and multi byte string.
// Set locale if converting to wchar_t from it's corresponding encoded value utf16/32 using C++ stl functions.
// Right now it is just converted using reinterpret_cast.
template <typename FromCharType, typename ToCharType>
struct StlStringConv
{
    const ToCharType *convert(const FromCharType *start) noexcept
    {
        static_assert(DependentFalseTypeValue<StlStringConv>, "Unsupported CharType conversion combination for stl only string conv");
        return nullptr;
    }
};

//////////////////////////////////////////////////////////////////////////
/// Generalized Implementations
//////////////////////////////////////////////////////////////////////////
template <>
template <typename Type>
FORCE_INLINE String ToStringImpl<WChar>::toString(Type &&value) noexcept
{
    return String(std::to_wstring(std::forward<Type>(value)));
}
template <>
template <typename Type>
FORCE_INLINE String ToStringImpl<AChar>::toString(Type &&value) noexcept
{
    return String(std::to_string(std::forward<Type>(value)));
}

// Code counter for UTF
// Count code points
// https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=IWS-AppendixA
FORCE_INLINE uint64 StringCodePointsHelper::count(const WChar *startChar) noexcept
{
    uint64 num = 0;
    if CONST_EXPR (std::is_same_v<Utf16, WCharEncodedType>)
    {
        while (const auto ch = *reinterpret_cast<const Utf16 *>(startChar++))
        {
            // Special characters(Supplementary planes) are in range 0xD800 to 0xDFFF so anything
            // not in this region is a character Higher word of 32bit utf-16 starts from 0xD800
            // and lower word of 32bit utf-16 starts from 0xDC00 So if a character is either
            // above or equal to 0xE000 or is below 0xDC00 it can be counted as a character
            // https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=IWS-AppendixA
            // https://en.wikipedia.org/wiki/UTF-16
            num += (ch < 0xDC00u) || (ch >= 0xE000u);
        }
    }
    else
    {
        while (const auto ch = *reinterpret_cast<const Utf32 *>(startChar++))
        {
            // Special characters(Supplementary planes) are in range 0xD800 to 0xDFFF so anything
            // not in this region is a character We must ignore those here but it is okay
            num++;
        }
    }
    return num;
}

FORCE_INLINE uint64 StringCodePointsHelper::count(const AChar *startChar) noexcept
{
    uint64 num = 0;
    while (const auto ch = *reinterpret_cast<const Utf8 *>(startChar++))
    {
        // If is less than 128(0x80) or greater than/equal to 192(0b11000xxx) then it means a
        // character https://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=IWS-AppendixA
        num += (ch >= 192u) || (ch < 128u);
    }
    return num;
}

FORCE_INLINE uint32 StringCodePointsHelper::utf8ToCode(const Utf8 *firstChar, uint32 byteCount) noexcept
{
    uint32 codePoint = 0;
    switch (byteCount)
    {
    case 1:
        codePoint = (uint32)(*firstChar);
        break;
    case 2:
        // 1st byte - Remove 0b110xxxxx from first byte and left shift by 6(*64)
        // 2nd byte - Remove 0b10xxxxxx from second byte
        codePoint = (*firstChar - 192u) * 64u + (*(firstChar + 1) - 128u);
        break;
    case 3:
        // 1st byte - Remove 0b1110xxxx(224u) from first byte and left shift by 12(*4096)
        // 2nd byte - Remove 0b10xxxxxx from second byte and left shift by 6(*64)
        // 3rd byte - Remove 0b10xxxxxx from third byte
        codePoint = (*firstChar - 224u) * 4096u + (*(firstChar + 1) - 128u) * 64u + (*(firstChar + 2) - 128u);
        break;
    case 4:
    default:
        // Handling anything above 4 bytes as 4 bytes and skip rest of the bytes

        // 1st byte - Remove 0b11110xxx(240u) from first byte and left shift by 18(*262144)
        // 2nd byte - Remove 0b10xxxxxx from second byte and left shift by 12(*4096)
        // 3rd byte - Remove 0b10xxxxxx from third byte and left shift by 6(*64)
        // 4th byte - Remove 0b10xxxxxx from fourth byte
        codePoint
            = (*firstChar - 240u) * 262144u + (*(firstChar + 1) - 128u) * 4096u + (*(firstChar + 2) - 128u) * 64u + (*(firstChar + 3) - 128u);
        break;
    }
    return codePoint;
}

FORCE_INLINE uint32 StringCodePointsHelper::utf16ToCode(const Utf16 *firstChar) noexcept
{
    // Single wide char
    if ((*firstChar < 0xD800u) && (*firstChar >= 0xE000u))
    {
        return (uint32)(*firstChar);
    }

    // HighWord - remove added 0xD800 specialization plane from higher word and left shift by 10(*0x400u)
    // LowWord - remove added 0xDC00 from lower word
    // Add back the 2^16(65536 chars) offset subtracted when encoding specialization plane to fit in
    // 20bits
    return 0x10000u + (*firstChar - 0xD800u) * 0x400u + (*(firstChar + 1) - 0xDC00u);
}

FORCE_INLINE uint32 StringCodePointsHelper::codePoint(AChar const *&endChar, AChar const *&startChar) noexcept
{
#if DEBUG_VALIDATIONS
    validateStartCode(*startChar);
#endif // DEBUG_VALIDATIONS

    // null terminated already?
    if (*startChar == '\0')
    {
        endChar = startChar;
        return 0;
    }

    // If less than 128 it is 1byte length
    if (*reinterpret_cast<const Utf8 *>(startChar) < 128u)
    {
        endChar = startChar + 1;
        return *startChar;
    }

    uint32 byteCount = 1;
    // This is multi-byte at this point, Iterate until end and find byteCount
    const Utf8 *firstChar = reinterpret_cast<const Utf8 *>(startChar + 1);
    while (*firstChar && (*firstChar < 192u && *firstChar >= 128u))
    {
        ++byteCount;
        ++firstChar;
    }
    endChar = (const AChar *)firstChar;

    return utf8ToCode(reinterpret_cast<const Utf8 *>(startChar), byteCount);
}

FORCE_INLINE uint32 StringCodePointsHelper::nextCodePoint(AChar const *&endChar, AChar const *&startChar) noexcept
{
    // Find the next char that has valid first byte, This avoids invalid char if startChar is from middle
    // of multi-byte Any character in range [128u, 192) is not valid start byte
    const Utf8 *firstChar = reinterpret_cast<const Utf8 *>(startChar);
    while (*firstChar && (*firstChar < 192u && *firstChar >= 128u))
    {
        ++firstChar;
    }
    startChar = (const AChar *)firstChar;

    return codePoint(endChar, startChar);
}

FORCE_INLINE uint32 StringCodePointsHelper::prevCodePoint(AChar const *&endChar, AChar const *&startChar) noexcept
{
    const Utf8 *firstChar = reinterpret_cast<const Utf8 *>(--startChar);
    while (*firstChar < 192u && *firstChar >= 128u)
    {
        --firstChar;
    }
    startChar = (const AChar *)firstChar;

    return codePoint(endChar, startChar);
}

FORCE_INLINE uint32 StringCodePointsHelper::codePoint(WChar const *&endChar, WChar const *&startChar) noexcept
{
#if DEBUG_VALIDATIONS
    validateStartCode(*startChar);
#endif // DEBUG_VALIDATIONS

    // null terminated already?
    if (*startChar == L'\0')
    {
        endChar = startChar;
        return 0;
    }

    endChar = startChar + 1;
    if CONST_EXPR (std::is_same_v<Utf16, WCharEncodedType>)
    {
        const Utf16 *firstChar = reinterpret_cast<const Utf16 *>(startChar);
        // Add one more byte if character is two word encoded
        endChar = endChar + ((*firstChar >= 0xD800u) && (*firstChar < 0xDC00u));
        return utf16ToCode(firstChar);
    }
    else
    {
        return (uint32)(*startChar);
    }
}

FORCE_INLINE uint32 StringCodePointsHelper::nextCodePoint(WChar const *&endChar, WChar const *&startChar) noexcept
{
    if CONST_EXPR (std::is_same_v<Utf16, WCharEncodedType>)
    {
        // First valid start must be either single word or word greater than or equal to 0xD800 and
        // Less than 0xDC00 (or must not be in range [0xDC00, 0xE000)
        const Utf16 *firstChar = reinterpret_cast<const Utf16 *>(startChar);
        while (*firstChar && !(*firstChar >= 0xDC00u && *firstChar < 0xE000u))
        {
            ++firstChar;
        }
        startChar = (const WChar *)firstChar;

        return codePoint(endChar, startChar);
    }
    else
    {
        endChar = startChar + 1;
        return (uint32)(*startChar);
    }
}

FORCE_INLINE uint32 StringCodePointsHelper::prevCodePoint(WChar const *&endChar, WChar const *&startChar) noexcept
{
    if CONST_EXPR (std::is_same_v<Utf16, WCharEncodedType>)
    {
        // First valid start must be either single word or word greater than or equal to 0xD800 and
        // Less than 0xDC00 (or must not be in range [0xDC00, 0xE000)
        const Utf16 *firstChar = reinterpret_cast<const Utf16 *>(--startChar);
        while (!(*firstChar >= 0xDC00u && *firstChar < 0xE000u))
        {
            --firstChar;
        }
        startChar = (const WChar *)firstChar;

        return codePoint(endChar, startChar);
    }
    else
    {
        endChar = startChar--;
        return (uint32)(*startChar);
    }
}

//////////////////////////////////////////////////////////////////////////
// Conversion between char types implementations
//////////////////////////////////////////////////////////////////////////

// For conversion between same char types
template <typename CharType>
struct StringConv<CharType, CharType>
{
private:
public:
    FORCE_INLINE const CharType *convert(const CharType *start) noexcept
    {
        // Does it pass the same pointer and keep the temporary converter if any valid?
        return start;
    }
};
// For conversion between platform WideChar and UTF-8
template <>
struct StringConv<WChar, AChar>
{
protected:
    std::string str{};

public:
    PROGRAMCORE_EXPORT const AChar *convert(const WChar *start) noexcept;
    // Just alias for above to allow converting from inherited StringConv<Utf16/32, Utf8>
    FORCE_INLINE const AChar *convert(const WCharEncodedType *start) noexcept { return convert(reinterpret_cast<const WChar *>(start)); }
};
template <>
struct StringConv<AChar, WChar>
{
protected:
    std::wstring str{};

public:
    PROGRAMCORE_EXPORT const WChar *convert(const AChar *start) noexcept;
};
// Conversion to UTF-8 between UTF-16
template <>
struct StlStringConv<AChar, Utf16>
{
protected:
    std::u16string str{};

public:
    PROGRAMCORE_EXPORT const Utf16 *convert(const AChar *start) noexcept;
};
template <>
struct StlStringConv<Utf16, AChar>
{
protected:
    std::string str{};

public:
    PROGRAMCORE_EXPORT const AChar *convert(const Utf16 *start) noexcept;
};
// Conversion to UTF-8 between UTF-32
template <>
struct StlStringConv<AChar, Utf32>
{
protected:
    std::u32string str{};

public:
    PROGRAMCORE_EXPORT const Utf32 *convert(const AChar *start) noexcept;
};
template <>
struct StlStringConv<Utf32, AChar>
{
protected:
    std::string str{};

public:
    PROGRAMCORE_EXPORT const AChar *convert(const Utf32 *start) noexcept;
};
// StringConv specialization for conversion between any encoded char type and UTF-8
template <typename FromCharType>
requires (!std::same_as<FromCharType, AChar>)
struct StringConv<FromCharType, AChar>
    : public std::conditional_t<std::is_same_v<FromCharType, WCharEncodedType>, StringConv<WChar, AChar>, StlStringConv<FromCharType, AChar>>
{};
template <typename ToCharType>
requires (!std::same_as<ToCharType, AChar>)
struct StringConv<AChar, ToCharType>
    : public std::conditional_t<std::is_same_v<ToCharType, WCharEncodedType>, StringConv<AChar, WChar>, StlStringConv<AChar, ToCharType>>
{};

//////////////////////////////////////////////////////////////////////////
/// String implementations
//////////////////////////////////////////////////////////////////////////

template <typename Type>
STRING_FUNCQUALIFIER String String::toString(Type &&value) noexcept
{
    return ToStringImpl<String::value_type>::toString(std::forward<Type>(value));
}

FORCE_INLINE uint64 String::codeCount() const noexcept { return StringCodePointsHelper::count(getChar()); }
// Code points iterator
// Lifetime must be less than the string that this is iterating
class StringCodePointsIterator
{
private:
    String::const_pointer beginPtr;
    String::const_pointer endPtr;
    String::const_pointer charStart;
    String::const_pointer charEnd;
    uint32 codePtValue;

public:
    /* Iterator traits skipped difference_type as it does not makes sense */
    using value_type = uint32;
    using reference = uint32 &;
    using pointer = uint32 *;
    using const_reference = const uint32 &;
    using const_pointer = const uint32 *;
    using iterator_category = std::bidirectional_iterator_tag;

    StringCodePointsIterator() = default;
    StringCodePointsIterator(const StringCodePointsIterator &itr) = default;
    StringCodePointsIterator(StringCodePointsIterator &&itr) = default;
    StringCodePointsIterator(const String &str) noexcept
        : beginPtr(str.getChar())
        , endPtr(str.getChar() + str.size())
        , charStart(str.getChar())
        , codePtValue(StringCodePointsHelper::codePoint(charEnd, charStart))
    {}

    // Helper to get end iterator for a String
    static StringCodePointsIterator end(const String &str) noexcept
    {
        StringCodePointsIterator itr(str);
        // Setting end will make next char to start from end
        itr.charEnd = itr.endPtr;
        return ++itr;
    }

    StringView view() const noexcept { return StringView(charStart, charEnd); }

    const_pointer operator->() const noexcept { return &codePtValue; }

    const value_type &operator* () const noexcept { return codePtValue; }

    bool operator!= (const StringCodePointsIterator &other) const noexcept
    {
        return !(codePtValue == other.codePtValue && charStart == other.charStart && charEnd == other.charEnd);
    }

    StringCodePointsIterator &operator++ () noexcept
    {
        // if we arrived at end just setup few things manually, calling nextCodePoint must give same
        // result with same input
        if (charEnd == endPtr)
        {
            charStart = charEnd = endPtr;
            codePtValue = 0;
        }
        else
        {
            charStart = charEnd;
            codePtValue = StringCodePointsHelper::nextCodePoint(charEnd, charStart);
        }
        return *this;
    }

    StringCodePointsIterator operator++ (int) noexcept
    {
        StringCodePointsIterator retVal(*this);
        this->operator++ ();
        return retVal;
    }

    StringCodePointsIterator &operator-- () noexcept
    {
        if (charStart != beginPtr)
        {
            codePtValue = StringCodePointsHelper::prevCodePoint(charEnd, charStart);
        }
        return *this;
    }

    StringCodePointsIterator operator-- (int) noexcept
    {
        StringCodePointsIterator retVal(*this);
        this->operator-- ();
        return retVal;
    }
};

struct StringCodePoints
{
    const String *str = nullptr;

    StringCodePoints(const String &inStr) noexcept
        : str(&inStr)
    {}

    StringCodePointsIterator begin() const noexcept
    {
        if (!str)
        {
            return StringCodePointsIterator();
        }
        return StringCodePointsIterator(*str);
    }

    StringCodePointsIterator end() const noexcept
    {
        if (!str)
        {
            return StringCodePointsIterator();
        }
        return StringCodePointsIterator::end(*str);
    }
};

#if USING_WIDE_UNICODE
#define COUT std::wcout
#define CERR std::wcerr
#else // USING_WIDE_UNICODE
#define COUT std::cout
#define CERR std::cerr
#endif // USING_WIDE_UNICODE

// Why typedef? to skip using comma inside macro as there is possiblity for us to chain below macros
// together
using StringConvTChar2AChar = StringConv<TChar, AChar>;
using StringConvAChar2TChar = StringConv<AChar, TChar>;
using StringConvUtf162AChar = StringConv<Utf16, AChar>;
using StringConvAChar2Utf16 = StringConv<AChar, Utf16>;
using StringConvUtf322AChar = StringConv<Utf32, AChar>;
using StringConvAChar2Utf32 = StringConv<AChar, Utf32>;

using StringConvWChar2TChar = StringConv<WChar, TChar>;
using StringConvWChar2AChar = StringConv<WChar, AChar>;
using StringConvAChar2WChar = StringConv<AChar, WChar>;
// Pass Char ptr of that type to convert from and AChar is always UTF8 encoded in our case, Do not store
// reference to returned pointer
#define TCHAR_TO_ANSI(TCharPtr) StringConvTChar2AChar{}.convert((const TChar *)TCharPtr)
#define TCHAR_TO_UTF8(TCharPtr) TCHAR_TO_ANSI(TCharPtr)
#define ANSI_TO_TCHAR(AnsiPtr) StringConvAChar2TChar{}.convert((const AChar *)AnsiPtr)
#define UTF8_TO_TCHAR(Utf8Ptr) ANSI_TO_TCHAR(Utf8Ptr)

#define UTF16_TO_UTF8(Utf16Ptr) StringConvUtf162AChar{}.convert((const Utf16 *)Utf16Ptr)
#define UTF8_TO_UTF16(Utf8Ptr) StringConvAChar2Utf16{}.convert((const AChar *)Utf8Ptr)
#define UTF32_TO_UTF8(Utf32Ptr) StringConvUtf322AChar{}.convert((const Utf32 *)Utf32Ptr)
#define UTF8_TO_UTF32(Utf8Ptr) StringConvAChar2Utf32{}.convert((const AChar *)Utf8Ptr)

#if USING_WIDE_UNICODE

#if PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) StringConvWChar2TChar{}.convert((const WChar *)(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) UTF8_TO_TCHAR(UTF32_TO_UTF8(Utf32Ptr))
#else // PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) UTF8_TO_TCHAR(UTF16_TO_UTF8(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) StringConvWChar2TChar{}.convert((const WChar *)(Utf32Ptr))
#endif // PLATFORM_WINDOWS

#define WCHAR_TO_TCHAR(WCharPtr) WCharPtr
#define TCHAR_TO_WCHAR(TCharPtr) TCharPtr

#else // USING_WIDE_UNICODE
#define UTF16_TO_TCHAR(Utf16Ptr) UTF16_TO_UTF8(Utf16Ptr)
#define UTF32_TO_TCHAR(Utf32Ptr) UTF32_TO_UTF8(Utf32Ptr)

#define WCHAR_TO_TCHAR(WCharPtr) StringConvWChar2AChar{}.convert((const WChar *)(WCharPtr))
#define TCHAR_TO_WCHAR(TCharPtr) StringConvAChar2WChar{}.convert((const TChar *)(TCharPtr))

#endif // USING_WIDE_UNICODE
