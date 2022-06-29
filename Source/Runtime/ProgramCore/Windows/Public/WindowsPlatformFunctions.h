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
private:
    static void bindCrtHandlesToStdHandles(bool bBindStdIn, bool bBindStdOut, bool bBindStdErr);

public:
    static LibPointer *openLibrary(const TChar *libName);
    static void releaseLibrary(const LibPointer *libraryHandle);
    static void *getProcAddress(const LibPointer *libraryHandle, const TChar *symName);
    static void getModuleInfo(void *processHandle, LibPointer *libraryHandle, LibraryData &moduleData);
    static bool isSame(const LibPointer *leftHandle, const LibPointer *rightHandle);

    static void *createProcess(const String &applicationPath, const String &cmdLine, const String &environment, const String &workingDirectory);
    static void *getCurrentProcessHandle();
    static void closeProcessHandle(void *handle);
    static void getAllModules(void *processHandle, LibPointerPtr *modules, uint32 &modulesSize);

    static void setConsoleForegroundColor(uint8 r, uint8 g, uint8 b);
    static bool hasAttachedConsole();
    static void setupAvailableConsole();
    static void detachCosole();

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