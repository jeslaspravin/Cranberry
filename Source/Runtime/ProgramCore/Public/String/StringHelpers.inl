/*!
 * \file StringHelpers.inl
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

template <typename CharType>
struct ToStringImpl
{
    using value_type = CharType;

    template <typename Type>
    static String toString(Type&& value)
    {
        static_assert(false, "Unsupported CharType to convert into");
        return {};
    }
};

// Remember we are only converting to UTF-8 when we convert to AChar or char or char8_t
// For now it is okay if ASCII/ANSI end up as garbage
// 
// Set locale if converting to wchar_t using C++ stl functions, Right now it is just converted using reinterpret_cast
template <typename FromCharType, typename ToCharType>
struct StringConv
{
    // End is inclusive of null-terminated character
    const ToCharType* convert(const FromCharType* start)
    {
        static_assert(false, "Unsupported CharType conversion combination");
        return nullptr;
    }
};

// Implementations
template <>
template <typename Type>
FORCE_INLINE String ToStringImpl<WChar>::toString(Type&& value)
{
    return String(std::to_wstring(std::forward<Type>(value)));
}
template <>
template <typename Type>
FORCE_INLINE String ToStringImpl<AChar>::toString(Type&& value)
{
    return String(std::to_string(std::forward<Type>(value)));
}

template <>
struct StringConv<WChar, AChar>
{
private:
    std::string str;
public:
    PROGRAMCORE_EXPORT const AChar* convert(const WChar* start);
};
template <>
struct StringConv<AChar, WChar>
{
private:
    String str;
public:
    PROGRAMCORE_EXPORT const WChar* convert(const AChar* start);
};
// Platform encoded type
template <>
struct StringConv<EncodedType, AChar>
{
private:
    StringConv<WChar, AChar> convertor;
public:
    FORCE_INLINE const AChar* convert(const EncodedType* start)
    {
        return convertor.convert(reinterpret_cast<const WChar*>(start));
    }
};
template <>
struct StringConv<AChar, EncodedType>
{
private:
    StringConv<AChar, WChar> convertor;
public:
    FORCE_INLINE const EncodedType* convert(const AChar* start)
    {
        return reinterpret_cast<const EncodedType*>(convertor.convert(start));
    }
};
// Exclusion of platform char encoded type
using NotEncodedCharType = std::conditional_t<std::is_same_v<EncodedType, Utf16>, Utf32, Utf16>;
template <>
struct StringConv<NotEncodedCharType, AChar>
{
private:
    std::string str;
public:
    PROGRAMCORE_EXPORT const AChar* convert(const NotEncodedCharType* start);
};
template <>
struct StringConv<AChar, NotEncodedCharType>
{
private:
    using StringType = std::basic_string<NotEncodedCharType, std::char_traits<NotEncodedCharType>, std::allocator<NotEncodedCharType>>;
    StringType str;
public:
    PROGRAMCORE_EXPORT const NotEncodedCharType* convert(const AChar* start);
};
template <typename CharType>
struct StringConv<CharType, CharType>
{
    // End is inclusive of null-terminated character
    FORCE_INLINE const CharType* convert(const CharType* start)
    {
        return start;
    }
};

// String implementations
#if USING_UNICODE
#define COUT std::wcout
#define CERR std::wcerr
#else // USING_UNICODE
#define COUT std::cout
#define CERR std::cerr
#endif // USING_UNICODE

// Pass Char ptr of that type to convert from and AChar is always UTF8 encoded in our case, Do not store reference to returned pointer
#define TCHAR_TO_ANSI(TCharPtr) StringConv<TChar, AChar>{}.convert((const TChar*)TCharPtr)
#define TCHAR_TO_UTF8(TCharPtr) TCHAR_TO_ANSI(TCharPtr)
#define ANSI_TO_TCHAR(AnsiPtr) StringConv<AChar, TChar>{}.convert((const AChar*)AnsiPtr)
#define UTF8_TO_TCHAR(Utf8Ptr) ANSI_TO_TCHAR(Utf8Ptr)

#define UTF16_TO_UTF8(Utf16Ptr) StringConv<Utf16, AChar>{}.convert((const Utf16*)Utf16Ptr)
#define UTF8_TO_UTF16(Utf8Ptr) StringConv<AChar, Utf16>{}.convert((const AChar*)Utf8Ptr)
#define UTF32_TO_UTF8(Utf32Ptr) StringConv<Utf32, AChar>{}.convert((const Utf32*)Utf32Ptr)
#define UTF8_TO_UTF32(Utf8Ptr) StringConv<AChar, Utf32>{}.convert((const AChar*)Utf8Ptr)
#if PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) StringConv<WChar, TChar>{}.convert(reinterpret_cast<const WChar*>(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) UTF8_TO_TCHAR(UTF32_TO_UTF8(Utf32Ptr))
#else // PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) UTF8_TO_TCHAR(UTF16_TO_UTF8(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) StringConv<WChar, TChar>{}.convert(reinterpret_cast<const WChar*>(Utf32Ptr))
#endif // PLATFORM_WINDOWS

template <typename Type>
FORCE_INLINE String String::toString(Type&& value)
{
    return ToStringImpl<String::value_type>::toString(std::forward<Type>(value));
}

