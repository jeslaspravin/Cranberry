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
#include "String/String.h"
#include "Types/CoreTypes.h"
#include "Types/Platform/GenericPlatformTypes.h"

struct CBEGuid;

template <typename PlatformClass>
class GenericPlatformFunctions
{

protected:
public:
    // Extensions are auto appended by api to platform default
    FORCE_INLINE static LibPointer *openLibrary(String libName) { return PlatformClass::openLibrary(libName); }

    FORCE_INLINE static void releaseLibrary(const LibPointer *libraryHandle) { PlatformClass::releaseLibrary(libraryHandle); }

    FORCE_INLINE static void *getProcAddress(const LibPointer *libraryHandle, String symName)
    {
        return PlatformClass::getProcAddress(libraryHandle, symName);
    }

    FORCE_INLINE static bool isSame(const LibPointer *leftHandle, const LibPointer *rightHandle)
    {
        return PlatformClass::isSame(leftHandle, rightHandle);
    }

    // Process related functions
    FORCE_INLINE static void *
        createProcess(const String &applicationPath, const String &cmdLine, const String &environment, const String &workingDirectory)
    {
        return PlatformClass::createProcess(applicationPath, cmdLine, environment, workingDirectory);
    }
    FORCE_INLINE static void *getCurrentProcessHandle() { return PlatformClass::getCurrentProcessHandle(); }
    FORCE_INLINE static void closeProcessHandle(void *handle) { PlatformClass::closeProcessHandle(handle); }

    FORCE_INLINE static void getAllModules(void *processHandle, LibPointerPtr *modules, uint32 &modulesSize)
    {
        PlatformClass::getAllModules(processHandle, modules, modulesSize);
    }

    FORCE_INLINE static void getModuleInfo(void *processHandle, LibPointer *libraryHandle, LibraryData &moduleData)
    {
        PlatformClass::getModuleInfo(processHandle, libraryHandle, moduleData);
    }

    // Console related functions
    FORCE_INLINE static bool hasAttachedConsole() { return PlatformClass::hasAttachedConsole(); }
    /**
     * Attaches to console/terminal that is available either in parent process or in executing IDE output and performs some initial setup
     * Does only setting to use console-virtual-terminal-sequence and UTF-8 if console is already present for this application
     */
    FORCE_INLINE static void setupAvailableConsole() { PlatformClass::setupAvailableConsole(); }
    FORCE_INLINE static void detachCosole() { PlatformClass::detachCosole(); }

    // Platform tools

    FORCE_INLINE static String getClipboard() { return PlatformClass::getClipboard(); }

    FORCE_INLINE static bool setClipboard(const String &text) { return PlatformClass::setClipboard(text); }

    // Utilities

    /*
     * We need to remove ref and const here as we use perfect forwarding and type will be either const appended l/r value reference
     */
    template <typename UnsignedType>
    requires std::unsigned_integral<std::remove_cvref_t<UnsignedType>>
    static uint32 getSetBitCount(UnsignedType &&value)
    {
        // Switch statement is not working as const_expr
        if CONST_EXPR (sizeof(value) == 1)
        {
            return PlatformClass::getSetBitCount(static_cast<uint8>(value));
        }
        else if CONST_EXPR (sizeof(value) == 2)
        {
            return PlatformClass::getSetBitCount(static_cast<uint16>(value));
        }
        else if CONST_EXPR (sizeof(value) == 4)
        {
            return PlatformClass::getSetBitCount(static_cast<uint32>(value));
        }
        else if CONST_EXPR (sizeof(value) == 8)
        {
            return PlatformClass::getSetBitCount(static_cast<uint64>(value));
        }
        else if CONST_EXPR (std::is_lvalue_reference_v<UnsignedType>)
        {
            uint32 count = 0;
            constexpr SizeT numOf64bits = sizeof(value) / sizeof(uint64);
            const uint64 *ptr = reinterpret_cast<const uint64 *>(&value);
            for (SizeT i = 0; i < numOf64bits; ++i)
            {
                count += PlatformClass::getSetBitCount(*ptr);
                ptr += 1;
            }
            if (constexpr SizeT fracOf64bits = (sizeof(value) % sizeof(uint64)))
            {
                const uint8 *bytePtr = reinterpret_cast<const uint8 *>(ptr);
                for (uint32 i = 0; i < fracOf64bits; ++i)
                {
                    count += PlatformClass::getSetBitCount(*bytePtr);
                    bytePtr += 1;
                }
            }
            return count;
        }
        else
        {
            static_assert(DependentFalseTypeValue<UnsignedType>, "Unsupported type! for getSetBitCount");
        }
        return 0;
    }

    static void createGUID(CBEGuid &outGuid) { PlatformClass::createGUID(outGuid); }

    static bool wcharToUtf8(std::string &outStr, const WChar *wChar) { return PlatformClass::wcharToUtf8(outStr, wChar); }

    static bool utf8ToWChar(std::wstring &outStr, const AChar *aChar) { return PlatformClass::utf8ToWChar(outStr, aChar); }

    static bool toUpper(AChar *inOutStr) { return PlatformClass::toUpper(inOutStr); }
    static bool toUpper(WChar *inOutStr) { return PlatformClass::toUpper(inOutStr); }
    static AChar toUpper(AChar ch) { return PlatformClass::toUpper(ch); }
    static WChar toUpper(WChar ch) { return PlatformClass::toUpper(ch); }

    static bool toLower(AChar *inOutStr) { return PlatformClass::toLower(inOutStr); }
    static bool toLower(WChar *inOutStr) { return PlatformClass::toLower(inOutStr); }
    static AChar toLower(AChar ch) { return PlatformClass::toLower(ch); }
    static WChar toLower(WChar ch) { return PlatformClass::toLower(ch); }
};
