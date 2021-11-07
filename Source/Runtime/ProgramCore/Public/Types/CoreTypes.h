#pragma once

class GenericPlatformTypes
{
public:

    typedef unsigned char uint8;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;
    typedef unsigned short int uint16;


    typedef signed char int8;
    typedef signed int int32;
    typedef signed long long int64;
    typedef signed short int int16;

    typedef wchar_t WChar;
    typedef char AChar;
    typedef WChar TChar;
    typedef uint8 Char8;
    typedef uint16 Char16;
    typedef uint32 Char32;

    typedef unsigned short word;
    typedef unsigned long dword;


    typedef union _uint64
    {
        struct
        {
            dword lowPart;
            dword highPart;
        };
        struct
        {
            dword lowPart;
            dword highPart;
        } u;
        uint64 quadPart;
    } UInt64;
};

typedef GenericPlatformTypes::uint8 uint8;
typedef GenericPlatformTypes::uint16 uint16;
typedef GenericPlatformTypes::uint32 uint32;
typedef GenericPlatformTypes::uint64 uint64;

typedef GenericPlatformTypes::int8 int8;
typedef GenericPlatformTypes::int16 int16;
typedef GenericPlatformTypes::int32 int32;
typedef GenericPlatformTypes::int64 int64;

typedef GenericPlatformTypes::AChar AChar;// Fixed width
typedef GenericPlatformTypes::WChar WChar;
typedef GenericPlatformTypes::TChar TChar;
typedef GenericPlatformTypes::Char8 Utf8;// Variable width
typedef GenericPlatformTypes::Char16 Utf16;// Variable width
typedef GenericPlatformTypes::Char32 Utf32; // Fixed width
typedef GenericPlatformTypes::Char16 Ucs2; // Fixed width

typedef GenericPlatformTypes::word word;
typedef GenericPlatformTypes::dword dword;

typedef GenericPlatformTypes::UInt64 UInt64;