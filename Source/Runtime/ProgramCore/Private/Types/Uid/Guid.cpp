/*!
 * \file Guid.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Types/Uid/Guid.h"
#include "String/StringFormat.h"
#include "Types/Platform/PlatformFunctions.h"

CBEGuid::CBEGuid(EInitType) { PlatformFunctions::createGUID(*this); }

String CBEGuid::toString(CBEGuid::EGuidFormat format /*= DWordWithHyphen*/) const
{
    switch (format)
    {
    case CBEGuid::DigitsOnly:
        return StringFormat::format(TCHAR("%08X%08X%08X%08X"), parts.a, parts.b, parts.c, parts.d);
        break;
    case CBEGuid::HexValues:
        // Hard coding prefix 0x instead of format specifier flag # as this will always be hex
        return StringFormat::format(
            TCHAR("{0x%08X,0x%04hX,0x%04hX,{0x%02hhX,0x%02hhX,0x%02hhX,0x%"
                  "02hhX,0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX}}"),
            parts.a, 
            _comps.b.words.highWord, _comps.b.words.lowWord, 
            _comps.c.bytes.b3, _comps.c.bytes.b2, _comps.c.bytes.b1, _comps.c.bytes.b0, 
            _comps.d.bytes.b3, _comps.d.bytes.b2, _comps.d.bytes.b1, _comps.d.bytes.b0
        );
        break;
    case CBEGuid::DigitsWithHyphen:
        return StringFormat::format(
            TCHAR("%08X-%04hX-%04hX-%04hX-%04hX%08X"), 
            parts.a, 
            _comps.b.words.highWord, _comps.b.words.lowWord, 
            _comps.c.words.highWord, _comps.c.words.lowWord, 
            parts.d
        );
        break;
    case CBEGuid::DigitsInBraces:
        return StringFormat::format(
            TCHAR("{%08X-%04hX-%04hX-%04hX-%04hX%08X}"), 
            parts.a, 
            _comps.b.words.highWord, _comps.b.words.lowWord, 
            _comps.c.words.highWord, _comps.c.words.lowWord, 
            parts.d
        );
        break;
    case CBEGuid::DigitsInParans:
        return StringFormat::format(
            TCHAR("(%08X-%04hX-%04hX-%04hX-%04hX%08X)"),
            parts.a, 
            _comps.b.words.highWord, _comps.b.words.lowWord, 
            _comps.c.words.highWord, _comps.c.words.lowWord, 
            parts.d
        );
        break;
    case CBEGuid::DWordWithHyphen:
    default:
        return StringFormat::format(TCHAR("%08X-%08X-%08X-%08X"), parts.a, parts.b, parts.c, parts.d);
        break;
    }
}

CBEGuid CBEGuid::parse(const String &str) { return parse(str.getChar(), str.length()); }

CBEGuid CBEGuid::parseFormat(const String &str, EGuidFormat format) { return parseFormat(str.getChar(), str.length(), format); }
