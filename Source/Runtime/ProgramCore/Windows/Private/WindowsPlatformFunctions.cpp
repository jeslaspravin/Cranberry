/*!
 * \file WindowsPlatformFunctions.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowsPlatformFunctions.h"
#include "Types/Time.h"
#include "WindowsCommonHeaders.h"
#include <Psapi.h>
#include <chrono>
#include <intrin.h>
#include <libloaderapi.h>
#include <ratio>
#include <wtypes.h>

struct WindowsLibHandle : public LibPointer
{
    HMODULE libHandle;
    bool bUnload;
    WindowsLibHandle(HMODULE handle, bool manualUnload)
        : libHandle(handle)
        , bUnload(manualUnload)
    {}

    ~WindowsLibHandle()
    {
        if (bUnload)
            WindowsPlatformFunctions::releaseLibrary(this);
    }
};

// as 100ns
using WindowsTimeDuration = std::chrono::duration<int64, std::ratio<1, 10'000'000>>;
// From https://stackoverflow.com/questions/6161776/convert-windows-filetime-to-second-in-unix-linux
// Difference in seconds between epoch time(1st Jan 1970) and windows time(1st Jan 1601)
#define WIN_2_EPOCH 11644473600ll

/* Time impl functions, No need to enclose in macro as this TU won't be included in other platforms */
template <typename Resolution>
TickRep fromPlatformTimeImpl(int64 platformTick)
{
    using namespace std::chrono;

    static const seconds winTimeToEpochDuration(WIN_2_EPOCH);
    return duration_cast<Resolution>(WindowsTimeDuration(platformTick) - winTimeToEpochDuration).count();
}
template <typename Resolution>
int64 toPlatformTimeImpl(TickRep timeTick)
{
    using namespace std::chrono;

    static const seconds winTimeToEpochDuration(WIN_2_EPOCH);
    return duration_cast<WindowsTimeDuration>(Resolution(timeTick) + winTimeToEpochDuration).count();
}
#undef WIN_2_EPOCH

template <typename Resolution>
TickRep fromPlatformTime(int64 platformTick);
template <typename Resolution>
int64 toPlatformTime(TickRep timeTick);
#define SPECIALIZE_FROM_PLATFORM_TIME(TimeResolution)                                                   \
    template <>                                                                                         \
    TickRep fromPlatformTime<TimeResolution>(int64 platformTick)                                        \
    {                                                                                                   \
        return ::fromPlatformTimeImpl<TimeResolution>(platformTick);                                    \
    }                                                                                                   \
    template <>                                                                                         \
    int64 toPlatformTime<TimeResolution>(TickRep timeTick)                                              \
    {                                                                                                   \
        return ::toPlatformTimeImpl<TimeResolution>(timeTick);                                          \
    }

// std::chrono::microseconds
SPECIALIZE_FROM_PLATFORM_TIME(std::chrono::microseconds)
SPECIALIZE_FROM_PLATFORM_TIME(std::chrono::nanoseconds)

#undef SPECIALIZE_FROM_PLATFORM_TIME

LibPointer *WindowsPlatformFunctions::openLibrary(String libName)
{
    // #TODO(Jeslas) : Improve this to handle dependent dlls here, Using
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/february/inside-windows-win32-portable-executable-file-format-in-detail
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/march/inside-windows-an-in-depth-look-into-the-win32-portable-executable-file-format-part-2
    WindowsLibHandle *handle = new WindowsLibHandle(LoadLibrary(libName.getChar()), true);

    if (!handle->libHandle)
    {
        delete handle;
        handle = nullptr;
    }
    return handle;
}

void WindowsPlatformFunctions::releaseLibrary(const LibPointer *libraryHandle)
{
    HMODULE handle = static_cast<const WindowsLibHandle *>(libraryHandle)->libHandle;
    if (handle)
    {
        FreeLibrary(handle);
    }
}

void *WindowsPlatformFunctions::getProcAddress(const LibPointer *libraryHandle, String symName)
{
    return GetProcAddress(static_cast<const WindowsLibHandle *>(libraryHandle)->libHandle,
        TCHAR_TO_UTF8(symName.getChar()));
}

void WindowsPlatformFunctions::getModuleInfo(
    void *processHandle, LibPointer *libraryHandle, LibraryData &moduleData)
{
    if (!libraryHandle)
    {
        return;
    }

    String::value_type temp[MAX_PATH];
    MODULEINFO mi;
    HMODULE module = static_cast<WindowsLibHandle *>(libraryHandle)->libHandle;

    GetModuleInformation(processHandle, module, &mi, sizeof(mi));
    moduleData.basePtr = mi.lpBaseOfDll;
    moduleData.moduleSize = mi.SizeOfImage;

    GetModuleFileNameEx(processHandle, module, temp, sizeof(temp));
    moduleData.imgName = temp;
    GetModuleBaseName(processHandle, module, temp, sizeof(temp));
    moduleData.name = temp;
}

bool WindowsPlatformFunctions::isSame(const LibPointer *leftHandle, const LibPointer *rightHandle)
{
    return static_cast<const WindowsLibHandle *>(leftHandle)->libHandle
           == static_cast<const WindowsLibHandle *>(rightHandle)->libHandle;
}

void *WindowsPlatformFunctions::getCurrentThreadHandle() { return ::GetCurrentThread(); }

void *WindowsPlatformFunctions::getCurrentProcessHandle() { return ::GetCurrentProcess(); }

void WindowsPlatformFunctions::closeProcessHandle(void *handle) { ::CloseHandle((HANDLE)handle); }

void WindowsPlatformFunctions::getAllModules(
    void *processHandle, LibPointerPtr *modules, uint32 &modulesSize)
{
    if (modules == nullptr)
    {
        DWORD modSize;
        HMODULE mod;
        EnumProcessModulesEx(processHandle, &mod, 1, &modSize, LIST_MODULES_64BIT);
        modulesSize = modSize / sizeof(mod);
    }
    else
    {
        std::vector<HMODULE> mods(modulesSize);
        EnumProcessModulesEx(
            processHandle, mods.data(), modulesSize, (LPDWORD)&modulesSize, LIST_MODULES_64BIT);
        uint32 totalInserts = 0;
        for (HMODULE mod : mods)
        {
            if (mod)
            {
                modules[totalInserts++] = new WindowsLibHandle(mod, false);
            }
        }
        modulesSize = totalInserts;
    }
}

String WindowsPlatformFunctions::getClipboard()
{
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE clipboardHnd = ::GetClipboardData(CF_UNICODETEXT);
    if (clipboardHnd == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    String clipboard{ WCHAR_TO_TCHAR((const WChar *)::GlobalLock(clipboardHnd)) };
    ::GlobalUnlock(clipboardHnd);
    ::CloseClipboard();
    return clipboard;
}

bool WindowsPlatformFunctions::setClipboard(const String &text)
{
    if (!::OpenClipboard(NULL))
        return false;

    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, (HANDLE)TCHAR_TO_WCHAR(text.getChar())) == NULL)
    {
        ::CloseClipboard();
        return false;
    }

#if 0
    const int wideCharLen = ::MultiByteToWideChar(CP_UTF8, 0, text.getChar(), -1, NULL, 0);
    HGLOBAL clipboardHnd = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wideCharLen * sizeof(WCHAR));
    if (clipboardHnd == NULL)
    {
        ::CloseClipboard();
        return false;
    }
    WCHAR* clipboardDataHnd = (WCHAR*)::GlobalLock(clipboardHnd);
    ::MultiByteToWideChar(CP_UTF8, 0, text.getChar(), -1, clipboardDataHnd, wideCharLen);
    ::GlobalUnlock(clipboardHnd);
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, clipboardHnd) == NULL)
    {
        ::GlobalFree(clipboardHnd);
        ::CloseClipboard();
        return false;
    }
#endif // Disabled
    ::CloseClipboard();
    return true;
}

uint32 WindowsPlatformFunctions::getSetBitCount(uint8 value)
{
    return uint32(::__popcnt16(uint16(value)));
}

uint32 WindowsPlatformFunctions::getSetBitCount(uint16 value) { return uint32(::__popcnt16(value)); }

uint32 WindowsPlatformFunctions::getSetBitCount(uint32 value) { return uint32(::__popcnt(value)); }

uint32 WindowsPlatformFunctions::getSetBitCount(uint64 value) { return uint32(::__popcnt64(value)); }

bool WindowsPlatformFunctions::createGUID(CBEGuid &outGuid)
{
    // GUID uses unsigned long which is of size 8 in GCC and Clang, Will that be a problem?
    // MSVC unsigned long is 4 bytes so doing below static assert
    static_assert(sizeof(GUID) == 16, "GUID is 16byte in current compiler");
    return ::CoCreateGuid((GUID *)(&outGuid)) == S_OK;
}

bool WindowsPlatformFunctions::wcharToUtf8(std::string &outStr, const WChar *wChar)
{
    int32 bufLen = ::WideCharToMultiByte(CP_UTF8, 0, wChar, -1, NULL, 0, NULL, NULL);
    outStr.resize(bufLen);
    bufLen = ::WideCharToMultiByte(CP_UTF8, 0, wChar, -1, outStr.data(), bufLen, NULL, NULL);

    if (bufLen == 0)
    {
        return false;
    }
    else
    {
        outStr.resize(bufLen);
        return true;
    }
}

bool WindowsPlatformFunctions::utf8ToWChar(std::wstring &outStr, const AChar *aChar)
{
    int32 bufLen = ::MultiByteToWideChar(CP_UTF8, 0, aChar, -1, NULL, 0);
    outStr.resize(bufLen);
    bufLen = ::MultiByteToWideChar(CP_UTF8, 0, aChar, -1, outStr.data(), bufLen);

    if (bufLen == 0)
    {
        return false;
    }
    else
    {
        outStr.resize(bufLen);
        return true;
    }
}

bool WindowsPlatformFunctions::toUpper(WChar *inOutStr)
{
    WChar *retVal = ::CharUpperW(inOutStr);
    return inOutStr == retVal;
}
bool WindowsPlatformFunctions::toUpper(AChar *inOutStr)
{
    AChar *retVal = ::CharUpperA(inOutStr);
    return inOutStr == retVal;
}
WChar WindowsPlatformFunctions::toUpper(WChar ch)
{
    dword processedLen = ::CharUpperBuffW(&ch, 1);
    return ch;
}
AChar WindowsPlatformFunctions::toUpper(AChar ch)
{
    dword processedLen = ::CharUpperBuffA(&ch, 1);
    return ch;
}

bool WindowsPlatformFunctions::toLower(WChar *inOutStr)
{
    WChar *retVal = ::CharLowerW(inOutStr);
    return inOutStr == retVal;
}
bool WindowsPlatformFunctions::toLower(AChar *inOutStr)
{
    AChar *retVal = ::CharLowerA(inOutStr);
    return inOutStr == retVal;
}
WChar WindowsPlatformFunctions::toLower(WChar ch)
{
    dword processedLen = ::CharLowerBuffW(&ch, 1);
    // return processedLen == 1;
    return ch;
}
AChar WindowsPlatformFunctions::toLower(AChar ch)
{
    dword processedLen = ::CharLowerBuffA(&ch, 1);
    return ch;
}
