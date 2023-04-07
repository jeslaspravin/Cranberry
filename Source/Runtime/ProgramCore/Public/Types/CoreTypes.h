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
#error "Platform not supported!"
#elif PLATFORM_APPLE
#error "Platform not supported!"
#endif

using uint8 = PlatformCoreTypes::uint8;
using uint16 = PlatformCoreTypes::uint16;
using uint32 = PlatformCoreTypes::uint32;
using uint64 = PlatformCoreTypes::uint64;

using int8 = PlatformCoreTypes::int8;
using int16 = PlatformCoreTypes::int16;
using int32 = PlatformCoreTypes::int32;
using int64 = PlatformCoreTypes::int64;

using AChar = PlatformCoreTypes::AChar; // Fixed width
using WChar = PlatformCoreTypes::WChar;
using TChar = PlatformCoreTypes::TChar;
using Char8 = PlatformCoreTypes::Utf8;   // Variable width
using Char16 = PlatformCoreTypes::Utf16; // Variable width
using Char32 = PlatformCoreTypes::Utf32; // Fixed width
using Utf8 = PlatformCoreTypes::Utf8;    // Variable width
using Utf16 = PlatformCoreTypes::Utf16;  // Variable width
using Utf32 = PlatformCoreTypes::Utf32;  // Fixed width
// using Ucs2 = PlatformCoreTypes::Ucs2; // Fixed width
// using Ucs4 = PlatformCoreTypes::Ucs4; // Fixed width
//  Encoding used in platform specific WChar type
using WCharEncodedType = PlatformCoreTypes::WCharEncodedType;
// Encoding used in engine for this platform
using EncodedType = PlatformCoreTypes::EncodedType;

using word = PlatformCoreTypes::word;
using dword = PlatformCoreTypes::dword;

using SizeT = PlatformCoreTypes::SizeT;
using SSizeT = PlatformCoreTypes::SSizeT;
using UPtrInt = PlatformCoreTypes::UPtrInt;
using PtrInt = PlatformCoreTypes::PtrInt;

using UInt64 = PlatformCoreTypes::UInt64;

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

STATIC_ASSERT(sizeof(SizeT) == 8);
STATIC_ASSERT(sizeof(SSizeT) == 8);
STATIC_ASSERT(sizeof(UPtrInt) == 8);
STATIC_ASSERT(sizeof(PtrInt) == 8);

STATIC_ASSERT(sizeof(UInt64) == 8);

#undef STATIC_ASSERT
#pragma pop_macro("STATIC_ASSERT")

#define FOR_EACH_CORE_TYPES_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)                                                        \
    FirstMacroName(uint8)                                                                                                                      \
    MacroName(uint16)                                                                                                                          \
    MacroName(uint32)                                                                                                                          \
    MacroName(uint64)                                                                                                                          \
    MacroName(int8)                                                                                                                            \
    MacroName(int16)                                                                                                                           \
    MacroName(int32)                                                                                                                           \
    MacroName(int64)                                                                                                                           \
    MacroName(float)                                                                                                                           \
    MacroName(double)                                                                                                                          \
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

enum EThreadSharing
{
    ThreadSharing_Exclusive,
    ThreadSharing_Shared
};

// Common type's traits

namespace CoreTypeTraits
{
template <class T, T Value>
struct IntegralType
{
    static constexpr T value = Value;

    using value_type = T;
    using type = IntegralType;

    constexpr operator value_type () const noexcept { return value; }
    constexpr value_type operator() () const noexcept { return value; }
};

template <bool Value>
using BoolType = IntegralType<bool, Value>;
using TrueType = BoolType<true>;
using FalseType = BoolType<false>;
template <typename T1, typename T2>
struct IsSame : FalseType
{};
template <typename T>
struct IsSame<T, T> : TrueType
{};

} // namespace CoreTypeTraits

struct IsTCharWide : CoreTypeTraits::IsSame<TChar, WChar>
{};