/*!
 * \file GenericPlatformCoreTypes.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

template <typename PlatformType>
class GenericPlatformCoreTypes
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
    // In case changing TChar type do not forget to change USING_UNICODE define
    typedef WChar TChar;
    typedef char8_t Utf8;
    typedef char16_t Utf16;
    typedef char32_t Utf32;
    typedef uint16 Ucs2;
    typedef uint32 Ucs4;
    typedef Utf32 EncodedType;

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