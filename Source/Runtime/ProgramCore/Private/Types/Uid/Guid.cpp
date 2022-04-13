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
#include "Types/Platform/PlatformFunctions.h"
#include "String/StringFormat.h"

CBEGuid::CBEGuid(EInitType)
{
    PlatformFunctions::createGUID(*this);
}

String CBEGuid::toString(CBEGuid::EGuidFormat format /*= DWordWithHyphen*/) const
{
    switch (format)
    {
    case CBEGuid::DigitsOnly:
        return StringFormat::format(TCHAR("%08X%08X%08X%08X"), a, b, c, d);
        break;
    case CBEGuid::HexValues:
        // Hard coding prefix 0x instead of format specifier flag # as this will always be hex
        return StringFormat::format(
            TCHAR("{0x%08X,0x%04hX,0x%04hX,{0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX,0x%02hhX}}")
            , a, _b.highWord, _b.lowWord, _c.b3, _c.b2, _c.b1, _c.b0, _d.b3, _d.b2, _d.b1, _d.b0);
        break;
    case CBEGuid::DigitsWithHyphen:
        return StringFormat::format(
            TCHAR("%08X-%04hX-%04hX-%04hX-%04hX%08X")
            , a, _b.highWord, _b.lowWord, _c.highWord, _c.lowWord, d);
        break;
    case CBEGuid::DigitsInBraces:
        return StringFormat::format(
            TCHAR("{%08X-%04hX-%04hX-%04hX-%04hX%08X}")
            , a, _b.highWord, _b.lowWord, _c.highWord, _c.lowWord, d);
        break;
    case CBEGuid::DigitsInParans:
        return StringFormat::format(
            TCHAR("(%08X-%04hX-%04hX-%04hX-%04hX%08X)")
            , a, _b.highWord, _b.lowWord, _c.highWord, _c.lowWord, d);
        break;
    case CBEGuid::DWordWithHyphen:
    default:
        return StringFormat::format(TCHAR("%08X-%08X-%08X-%08X"), a, b, c, d);
        break;
    }
}

CBEGuid CBEGuid::parse(const String& str)
{
    return parse(str.getChar(), str.length());
}

CBEGuid CBEGuid::parseFormat(const String& str, EGuidFormat format)
{
    return parseFormat(str.getChar(), str.length(), format);
}
