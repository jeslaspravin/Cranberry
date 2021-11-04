#pragma once


#if _WIN32

#include "Windows/WindowsPlatformTypes.h"

#elif __unix__
static_assert(false, "Platform not supported!");
#elif __linux__
static_assert(false, "Platform not supported!");
#elif __APPLE__
static_assert(false, "Platform not supported!");
#endif

typedef PlatformTypes::uint8 uint8;
typedef PlatformTypes::uint16 uint16;
typedef PlatformTypes::uint32 uint32;
typedef PlatformTypes::uint64 uint64;

typedef PlatformTypes::int8 int8;
typedef PlatformTypes::int16 int16;
typedef PlatformTypes::int32 int32;
typedef PlatformTypes::int64 int64;

typedef PlatformTypes::AChar AChar;// Fixed width
typedef PlatformTypes::WChar WChar;
typedef PlatformTypes::TChar TChar;
typedef PlatformTypes::Char8 Utf8;// Variable width
typedef PlatformTypes::Char16 Utf16;// Variable width
typedef PlatformTypes::Char32 Utf32; // Fixed width
typedef PlatformTypes::Char16 Ucs2; // Fixed width

typedef PlatformTypes::word word;
typedef PlatformTypes::dword dword;

typedef PlatformTypes::UInt64 UInt64;