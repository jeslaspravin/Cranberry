/*!
 * \file CoreTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#if PLATFORM_WINDOWS
#include "WindowsCoreTypes.h"
#elif PLATFORM_LINUX
static_assert(false, "Platform not supported!");
#elif PLATFORM_APPLE
static_assert(false, "Platform not supported!");
#endif


typedef PlatformCoreTypes::uint8 uint8;
typedef PlatformCoreTypes::uint16 uint16;
typedef PlatformCoreTypes::uint32 uint32;
typedef PlatformCoreTypes::uint64 uint64;

typedef PlatformCoreTypes::int8 int8;
typedef PlatformCoreTypes::int16 int16;
typedef PlatformCoreTypes::int32 int32;
typedef PlatformCoreTypes::int64 int64;

typedef PlatformCoreTypes::AChar AChar;// Fixed width
typedef PlatformCoreTypes::WChar WChar;
typedef PlatformCoreTypes::TChar TChar;
typedef PlatformCoreTypes::Utf8 Char8;// Variable width
typedef PlatformCoreTypes::Utf16 Char16;// Variable width
typedef PlatformCoreTypes::Utf32 Char32; // Fixed width
typedef PlatformCoreTypes::Utf8 Utf8;// Variable width
typedef PlatformCoreTypes::Utf16 Utf16;// Variable width
typedef PlatformCoreTypes::Utf32 Utf32; // Fixed width
//typedef PlatformCoreTypes::Ucs2 Ucs2; // Fixed width
//typedef PlatformCoreTypes::Ucs4 Ucs4; // Fixed width
// Encoding used in platform specific WChar type
typedef PlatformCoreTypes::WCharEncodedType WCharEncodedType;
// Encoding used in engine for this platform
typedef PlatformCoreTypes::EncodedType EncodedType;

typedef PlatformCoreTypes::word word;
typedef PlatformCoreTypes::dword dword;

typedef PlatformCoreTypes::SizeT SizeT;
typedef PlatformCoreTypes::SSizeT SSizeT;
typedef PlatformCoreTypes::UIntPtr UIntPtr;
typedef PlatformCoreTypes::IntPtr IntPtr;

typedef PlatformCoreTypes::UInt64 UInt64;

// We do not want to clear all previous STATIC_ASSERTS
#pragma push_macro("STATIC_ASSERT")
#define STATIC_ASSERT(Stmt) static_assert((Stmt), #Stmt)

STATIC_ASSERT(sizeof(uint8) == 1);
STATIC_ASSERT(sizeof(uint16) == 2);
STATIC_ASSERT(sizeof(uint32) == 4);
STATIC_ASSERT(sizeof(uint64) == 8);

STATIC_ASSERT(sizeof(int8) == 1);
STATIC_ASSERT(sizeof(int16) == 2);
STATIC_ASSERT(sizeof(int32) == 4);
STATIC_ASSERT(sizeof(int64) == 8);

STATIC_ASSERT(sizeof(AChar) == 1);
// Depends on platform STATIC_ASSERT(sizeof(WChar) == 2); STATIC_ASSERT(sizeof(TChar) == 4);
STATIC_ASSERT(sizeof(Utf8) == 1);
STATIC_ASSERT(sizeof(Utf16) == 2);
STATIC_ASSERT(sizeof(Utf32) == 4);
// STATIC_ASSERT(sizeof(Ucs2) == 2);
STATIC_ASSERT(sizeof(WChar) == sizeof(WCharEncodedType));

STATIC_ASSERT(sizeof(word) == 2);
STATIC_ASSERT(sizeof(dword) == 4);

STATIC_ASSERT(sizeof(UInt64) == 8);

#undef STATIC_ASSERT
#pragma pop_macro("STATIC_ASSERT")

#define FOR_EACH_CORE_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \
    FirstMacroName(uint8) \
    MacroName(uint16)\
    MacroName(uint32)\
    MacroName(uint64)\
    MacroName(int8) \
    MacroName(int16)\
    MacroName(int32)\
    MacroName(int64)\
    MacroName(float) \
    MacroName(double)\
    LastMacroName(bool)

#define FOR_EACH_CORE_TYPES(MacroName) FOR_EACH_CORE_TYPES_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

#define DECLARE_CORE_TYPE_VAR(Type) Type Type##Val;
union CoreTypesUnion
{
    FOR_EACH_CORE_TYPES(DECLARE_CORE_TYPE_VAR)
};
#undef DECLARE_CORE_TYPE_VAR

enum EInitType
{
    InitType_DefaultInit,
    InitType_ForceInit,
    InitType_NoInit
};