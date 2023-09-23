/*!
 * \file GenericPlatformCoreTypes.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

class GenericPlatformCoreTypes
{
public:
    using uint8 = unsigned char;
    using uint32 = unsigned int;
    using uint64 = unsigned long long;
    using uint16 = unsigned short int;

    using int8 = signed char;
    using int32 = signed int;
    using int64 = signed long long;
    using int16 = signed short int;

    using WChar = wchar_t;
    using AChar = char;
    // In case changing TChar type do not forget to change USING_WIDE_UNICODE define
    // Also change EncodedType to new appropriate type
    using TChar = WChar;
    using Utf8 = char8_t;
    using Utf16 = char16_t;
    using Utf32 = char32_t;
    using Ucs2 = uint16;
    using Ucs4 = uint32;
    using WCharEncodedType = Utf32;
    using EncodedType = Utf32;

    using word = unsigned short;
    using dword = unsigned long;

    // Since we support only 64bit
    using SizeT = size_t;
    using SSizeT = int64;
    using UPtrInt = uint64;
    using PtrInt = int64;

    union UInt64
    {
#if (defined BIG_ENDIAN) & BIG_ENDIAN
        struct
        {
            dword highPart;
            dword lowPart;
        } dwords;
#else
        struct
        {
            dword lowPart;
            dword highPart;
        } dwords;
#endif
        uint64 quadPart;
    };
};