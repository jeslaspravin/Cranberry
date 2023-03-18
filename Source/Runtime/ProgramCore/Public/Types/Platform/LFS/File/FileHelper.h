/*!
 * \file FileHelper.h
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#pragma once

#include "ProgramCoreExports.h"
#include "String/String.h"

class PROGRAMCORE_EXPORT FileHelper
{
private:
    FileHelper() = default;

public:
    static bool isUtf8(const uint8 *byteStream, SizeT streamSize);
    static bool isUtf8Bom(const uint8 *byteStream, SizeT streamSize);
    static bool isUtf16LEBom(const uint8 *byteStream, SizeT streamSize);
    static bool isUtf16BEBom(const uint8 *byteStream, SizeT streamSize);
    static bool isUtf32LEBom(const uint8 *byteStream, SizeT streamSize);
    static bool isUtf32BEBom(const uint8 *byteStream, SizeT streamSize);

    static uint16 bytesSwap(uint16 value);
    static uint32 bytesSwap(uint32 value);
    static uint64 bytesSwap(uint64 value);
    static void bytesSwap(void *ptr, SizeT length);

    static bool readString(String &outStr, const String &fileName);
    static bool readUtf8String(std::string &outStr, const String &fileName);
    static bool readBytes(std::vector<uint8> &outBytes, const String &fileName);
    // Always writes to new file, overwrites if existing
    static bool writeString(const String &content, const String &fileName);
    static bool writeBytes(const std::vector<uint8> &bytes, const String &fileName);

    static bool touchFile(const String &fileName);
};