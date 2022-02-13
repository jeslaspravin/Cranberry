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

// Converter that uses STL to convert between an encoded format(UTF-16/UTF-32) and UTF-8, This is not for converting between platform string and multi byte string
// Set locale if converting to wchar_t from it's corresponding encoded value utf16/32 using C++ stl functions, Right now it is just converted using reinterpret_cast
template <typename FromCharType, typename ToCharType>
struct StlStringConv
{
    const ToCharType* convert(const FromCharType* start)
    {
        static_assert(false, "Unsupported CharType conversion combination for stl only string conv");
        return nullptr;
    }
};


//////////////////////////////////////////////////////////////////////////
/// Generalized Implementations
//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
// Conversion between char types implementations
//////////////////////////////////////////////////////////////////////////

// For conversion between same char types
template <typename CharType>
struct StringConv<CharType, CharType>
{
private:
public:
    FORCE_INLINE const CharType* convert(const CharType* start)
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
    PROGRAMCORE_EXPORT const AChar* convert(const WChar* start);
    // Just alias for above to allow converting from inherited StringConv<Utf16/32, Utf8>
    FORCE_INLINE const AChar* convert(const WCharEncodedType* start)
    {
        return convert(reinterpret_cast<const WChar*>(start));
    }
};
template <>
struct StringConv<AChar, WChar>
{
protected:
    std::wstring str{};
public:
    PROGRAMCORE_EXPORT const WChar* convert(const AChar* start);
};
// Conversion to UTF-8 between UTF-16
template <>
struct StlStringConv<AChar, Utf16>
{
protected:
    std::u16string str{};
public:
    PROGRAMCORE_EXPORT const Utf16* convert(const AChar* start);
};
template <>
struct StlStringConv<Utf16, AChar>
{
protected:
    std::string str{};
public:
    PROGRAMCORE_EXPORT const AChar* convert(const Utf16* start);
};
// Conversion to UTF-8 between UTF-32
template <>
struct StlStringConv<AChar, Utf32>
{
protected:
    std::u32string str{};
public:
    PROGRAMCORE_EXPORT const Utf32* convert(const AChar* start);
};
template <>
struct StlStringConv<Utf32, AChar>
{
protected:
    std::string str{};
public:
    PROGRAMCORE_EXPORT const AChar* convert(const Utf32* start);
};
// StringConv specialization for conversion between any encoded char type and UTF-8
template <typename FromCharType> requires (!std::same_as<FromCharType, AChar>)
struct StringConv<FromCharType, AChar> 
    : public std::conditional_t<
    std::is_same_v<FromCharType, WCharEncodedType>// ?
    , StringConv<WChar, AChar> // :
    , StlStringConv<FromCharType, AChar>>
{};
template <typename ToCharType> requires (!std::same_as<ToCharType, AChar>)
struct StringConv<AChar, ToCharType>
    : public std::conditional_t<
    std::is_same_v<ToCharType, WCharEncodedType>// ?
    , StringConv<AChar, WChar> // :
    , StlStringConv<AChar, ToCharType>>
{};

//////////////////////////////////////////////////////////////////////////
/// String implementations
//////////////////////////////////////////////////////////////////////////

template <typename Type>
FORCE_INLINE String String::toString(Type&& value)
{
    return ToStringImpl<String::value_type>::toString(std::forward<Type>(value));
}

#if USING_WIDE_UNICODE
#define COUT std::wcout
#define CERR std::wcerr
#else // USING_WIDE_UNICODE
#define COUT std::cout
#define CERR std::cerr
#endif // USING_WIDE_UNICODE

// Pass Char ptr of that type to convert from and AChar is always UTF8 encoded in our case, Do not store reference to returned pointer
#define TCHAR_TO_ANSI(TCharPtr) StringConv<TChar, AChar>{}.convert((const TChar*)TCharPtr)
#define TCHAR_TO_UTF8(TCharPtr) TCHAR_TO_ANSI(TCharPtr)
#define ANSI_TO_TCHAR(AnsiPtr) StringConv<AChar, TChar>{}.convert((const AChar*)AnsiPtr)
#define UTF8_TO_TCHAR(Utf8Ptr) ANSI_TO_TCHAR(Utf8Ptr)

#define UTF16_TO_UTF8(Utf16Ptr) StringConv<Utf16, AChar>{}.convert((const Utf16*)Utf16Ptr)
#define UTF8_TO_UTF16(Utf8Ptr) StringConv<AChar, Utf16>{}.convert((const AChar*)Utf8Ptr)
#define UTF32_TO_UTF8(Utf32Ptr) StringConv<Utf32, AChar>{}.convert((const Utf32*)Utf32Ptr)
#define UTF8_TO_UTF32(Utf8Ptr) StringConv<AChar, Utf32>{}.convert((const AChar*)Utf8Ptr)

#if USING_WIDE_UNICODE

#if PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) StringConv<WChar, TChar>{}.convert((const WChar*)(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) UTF8_TO_TCHAR(UTF32_TO_UTF8(Utf32Ptr))
#else // PLATFORM_WINDOWS
#define UTF16_TO_TCHAR(Utf16Ptr) UTF8_TO_TCHAR(UTF16_TO_UTF8(Utf16Ptr))
#define UTF32_TO_TCHAR(Utf32Ptr) StringConv<WChar, TChar>{}.convert((const WChar*)(Utf32Ptr))
#endif // PLATFORM_WINDOWS

#define WCHAR_TO_TCHAR(WCharPtr) WCharPtr
#define TCHAR_TO_WCHAR(TCharPtr) TCharPtr

#else // USING_WIDE_UNICODE
#define UTF16_TO_TCHAR(Utf16Ptr) UTF16_TO_UTF8(Utf16Ptr)
#define UTF32_TO_TCHAR(Utf32Ptr) UTF32_TO_UTF8(Utf32Ptr)

#define WCHAR_TO_TCHAR(WCharPtr) StringConv<WChar, AChar>{}.convert((const WChar*)(WCharPtr))
#define TCHAR_TO_WCHAR(TCharPtr) StringConv<AChar, WChar>{}.convert((const TChar*)(TCharPtr))

#endif // USING_WIDE_UNICODE
