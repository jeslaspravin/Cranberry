/*!
 * \file WindowsPlatformFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/Platform/GenericPlatformFunctions.h"

class PROGRAMCORE_EXPORT WindowsPlatformFunctions : public GenericPlatformFunctions<WindowsPlatformFunctions>
{
private:
    static void bindCrtHandlesToStdHandles(bool bBindStdIn, bool bBindStdOut, bool bBindStdErr);

public:
    static LibHandle openLibrary(const TChar *libName);
    static void releaseLibrary(LibHandle libraryHandle);
    static ProcAddress getProcAddress(LibHandle libraryHandle, const TChar *symName);
    static void getModuleInfo(PlatformHandle processHandle, LibHandle libraryHandle, LibraryData &moduleData);

    static PlatformHandle
    createProcess(const String &applicationPath, const String &cmdLine, const String &environment, const String &workingDirectory);
    static PlatformHandle getCurrentProcessHandle();
    static void closeProcessHandle(PlatformHandle handle);

    static void getAllModules(PlatformHandle processHandle, LibHandle *modules, uint32 &modulesSize);
    static LibHandle getAddressModule(void *address);

    static void setConsoleForegroundColor(uint8 r, uint8 g, uint8 b);
    static bool hasAttachedConsole();
    static void setupAvailableConsole();
    static void detachCosole();

    static bool hasAttachedDebugger();
    static void outputToDebugger(const TChar *msg) noexcept;

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