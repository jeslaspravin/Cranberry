/*!
 * \file GenericPlatformFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"
#include "String/String.h"
#include "Types/Platform/GenericPlatformTypes.h"

struct CBEGuid;

template <typename PlatformClass>
class GenericPlatformFunctions
{

protected:
public:
    // Extensions are auto appended by api to platform default
    static LibPointer* openLibrary(String libName)
    {
        return PlatformClass::openLibrary(libName);
    }

    static void releaseLibrary(const LibPointer* libraryHandle)
    {
        PlatformClass::releaseLibrary(libraryHandle);
    }

    static void* getProcAddress(const LibPointer* libraryHandle, String symName)
    {
        return PlatformClass::getProcAddress(libraryHandle, symName);
    }

    static bool isSame(const LibPointer* leftHandle, const LibPointer* rightHandle)
    {
        return PlatformClass::isSame(leftHandle, rightHandle);
    }

    static void* getCurrentThreadHandle()
    {
        return PlatformClass::getCurrentThreadHandle();
    }

    static void* getCurrentProcessHandle()
    {
        return PlatformClass::getCurrentProcessHandle();
    }
    static void closeProcessHandle(void* handle)
    {
        PlatformClass::closeProcessHandle(handle);
    }

    static void getAllModules(void* processHandle, LibPointerPtr* modules,uint32& modulesSize)
    {
        PlatformClass::getAllModules(processHandle, modules, modulesSize);
    }

    static void getModuleInfo(void* processHandle, LibPointer* libraryHandle, LibraryData& moduleData)
    {
        PlatformClass::getModuleInfo(processHandle, libraryHandle, moduleData);
    }

    // Platform tools

    static String getClipboard()
    {
        return PlatformClass::getClipboard();
    }

    static bool setClipboard(const String& text)
    {
        return PlatformClass::setClipboard(text);
    }

    // Utilities 

    template<std::unsigned_integral UnsignedType>
    static uint32 getSetBitCount(const UnsignedType& value)
    {
        return PlatformClass::getSetBitCount(value);
    }

    static void createGUID(CBEGuid& outGuid)
    {
        PlatformClass::createGUID(outGuid);
    }

    static bool wcharToUtf8(std::string& outStr, const WChar* wChar)
    {
        return PlatformClass::wcharToUtf8(outStr, wChar);
    }

    static bool utf8ToWChar(std::wstring& outStr, const AChar* aChar)
    {
        return PlatformClass::utf8ToWChar(outStr, aChar);
    }

    static bool toUpper(AChar* inOutStr)
    {
        return PlatformClass::toUpper(inOutStr);
    }
    static bool toUpper(WChar* inOutStr)
    {
        return PlatformClass::toUpper(inOutStr);
    }
    static AChar toUpper(AChar ch)
    {
        return PlatformClass::toUpper(ch);
    }
    static WChar toUpper(WChar ch)
    {
        return PlatformClass::toUpper(ch);
    }

    static bool toLower(AChar* inOutStr)
    {
        return PlatformClass::toLower(inOutStr);
    }
    static bool toLower(WChar* inOutStr)
    {
        return PlatformClass::toLower(inOutStr);
    }
    static AChar toLower(AChar ch)
    {
        return PlatformClass::toLower(ch);
    }
    static WChar toLower(WChar ch)
    {
        return PlatformClass::toLower(ch);
    }
};
