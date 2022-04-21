/*!
 * \file WindowsPlatformFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/Platform/GenericPlatformFunctions.h"
#include "Types/Platform/GenericPlatformTypes.h"

class PROGRAMCORE_EXPORT WindowsPlatformFunctions : public GenericPlatformFunctions<WindowsPlatformFunctions>
{

public:
    static LibPointer *openLibrary(String libName);
    static void releaseLibrary(const LibPointer *libraryHandle);
    static void *getProcAddress(const LibPointer *libraryHandle, String symName);
    static void getModuleInfo(void *processHandle, LibPointer *libraryHandle, LibraryData &moduleData);
    static bool isSame(const LibPointer *leftHandle, const LibPointer *rightHandle);

    static void *getCurrentThreadHandle();
    static void *getCurrentProcessHandle();
    static void closeProcessHandle(void *handle);
    static void getAllModules(void *processHandle, LibPointerPtr *modules, uint32 &modulesSize);

    static String getClipboard();
    static bool setClipboard(const String &text);

    static uint32 getSetBitCount(uint8 value);
    static uint32 getSetBitCount(uint16 value);
    static uint32 getSetBitCount(uint32 value);
    static uint32 getSetBitCount(uint64 value);

    static bool createGUID(CBEGuid &outGuid);

    static bool wcharToUtf8(std::string &outStr, const WChar *wChar);
    static bool utf8ToWChar(std::wstring &outStr, const AChar *aChar);

    static bool toUpper(AChar *inOutStr);
    static bool toUpper(WChar *inOutStr);
    static AChar toUpper(AChar ch);
    static WChar toUpper(WChar ch);

    static bool toLower(AChar *inOutStr);
    static bool toLower(WChar *inOutStr);
    static AChar toLower(AChar ch);
    static WChar toLower(WChar ch);
};

namespace GPlatformFunctions
{
typedef GenericPlatformFunctions<WindowsPlatformFunctions> PlatformFunctions;
}