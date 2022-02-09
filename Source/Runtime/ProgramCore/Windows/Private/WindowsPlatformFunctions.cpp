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
#include "WindowsCommonHeaders.h"
#include "Types/Time.h"
#include <chrono>
#include <ratio>
#include <wtypes.h>
#include <libloaderapi.h>
#include <Psapi.h>
#include <intrin.h>

struct WindowsLibHandle : public LibPointer
{
    HMODULE libHandle;
    bool bUnload;
    WindowsLibHandle(HMODULE handle,bool manualUnload) :libHandle(handle),bUnload(manualUnload) {}

    ~WindowsLibHandle() {
        if(bUnload)
            WindowsPlatformFunctions::releaseLibrary(this);
    }
};

/* Time impl functions, No need to enclose in macro as this TU won't be included in other platforms */
template <typename Resolution>
TickRep fromPlatformTimeImpl(int64 platformTick)
{
    using namespace std::chrono;
    using WindowsTimeDuration = duration<int64, std::ratio<1, 10'000'000>>;

    // From https://stackoverflow.com/questions/6161776/convert-windows-filetime-to-second-in-unix-linux
    // Difference in seconds between epoch time(1st Jan 1970) and windows time(1st Jan 1601)
    static const seconds winTimeToEpochDuration(11644473600ll);
    return duration_cast<Resolution>(WindowsTimeDuration(platformTick) - winTimeToEpochDuration).count();
}

template <typename Resolution>
TickRep fromPlatformTime(int64 platformTick);
#define SPECIALIZE_FROM_PLATFORM_TIME(TimeResolution) \
template <> \
TickRep fromPlatformTime<TimeResolution>(int64 platformTick) { return ::fromPlatformTimeImpl<TimeResolution>(platformTick); }
// std::chrono::microseconds
SPECIALIZE_FROM_PLATFORM_TIME(std::chrono::microseconds)
SPECIALIZE_FROM_PLATFORM_TIME(std::chrono::nanoseconds)
#undef SPECIALIZE_FROM_PLATFORM_TIME

LibPointer* WindowsPlatformFunctions::openLibrary(String libName)
{
    // #TODO(Jeslas) : Improve this to handle dependent dlls here, Using
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/february/inside-windows-win32-portable-executable-file-format-in-detail
    // https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/march/inside-windows-an-in-depth-look-into-the-win32-portable-executable-file-format-part-2
    WindowsLibHandle* handle = new WindowsLibHandle(LoadLibrary(libName.getChar()), true);

    if (!handle->libHandle)
    {
        delete handle;
        handle = nullptr;
    }
    return handle;
}

void WindowsPlatformFunctions::releaseLibrary(const LibPointer* libraryHandle)
{
    HMODULE handle = static_cast<const WindowsLibHandle*>(libraryHandle)->libHandle;
    if (handle) 
    {
        FreeLibrary(handle);
    }
}

void* WindowsPlatformFunctions::getProcAddress(const LibPointer* libraryHandle, String symName)
{
    return GetProcAddress(static_cast<const WindowsLibHandle*>(libraryHandle)->libHandle, TCHAR_TO_UTF8(symName.getChar()));
}

void WindowsPlatformFunctions::getModuleInfo(void* processHandle, LibPointer* libraryHandle, LibraryData& moduleData)
{
    if (!libraryHandle)
    {
        return;
    }

    String::value_type temp[MAX_PATH];
    MODULEINFO mi;
    HMODULE module = static_cast<WindowsLibHandle*>(libraryHandle)->libHandle;

    GetModuleInformation(processHandle,module,&mi, sizeof(mi));
    moduleData.basePtr = mi.lpBaseOfDll;
    moduleData.moduleSize = mi.SizeOfImage;

    GetModuleFileNameEx(processHandle, module, temp, sizeof(temp));
    moduleData.imgName = temp;
    GetModuleBaseName(processHandle, module, temp, sizeof(temp));
    moduleData.name = temp;
}

bool WindowsPlatformFunctions::isSame(const LibPointer* leftHandle, const LibPointer* rightHandle)
{
    return static_cast<const WindowsLibHandle*>(leftHandle)->libHandle == static_cast<const WindowsLibHandle*>
        (rightHandle)->libHandle;
}

void* WindowsPlatformFunctions::getCurrentThreadHandle()
{
    return ::GetCurrentThread();
}

void* WindowsPlatformFunctions::getCurrentProcessHandle()
{
    return ::GetCurrentProcess();
}

void WindowsPlatformFunctions::closeProcessHandle(void* handle)
{
    ::CloseHandle((HANDLE)handle);
}

void WindowsPlatformFunctions::getAllModules(void* processHandle, LibPointerPtr* modules, uint32& modulesSize)
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
        EnumProcessModulesEx(processHandle, mods.data(), modulesSize, (LPDWORD)&modulesSize, LIST_MODULES_64BIT);
        uint32 totalInserts = 0;
        for (HMODULE mod : mods)
        {
            if (mod)
            {
                modules[totalInserts++] = new WindowsLibHandle(mod,false);
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
    String clipboard{ (const WChar*)::GlobalLock(clipboardHnd) };
    ::GlobalUnlock(clipboardHnd);
    ::CloseClipboard();
    return clipboard;
}

bool WindowsPlatformFunctions::setClipboard(const String& text)
{
    if (!::OpenClipboard(NULL))
        return false;
#if USING_UNICODE
    ::EmptyClipboard();
    if (::SetClipboardData(CF_UNICODETEXT, (HANDLE)text.getChar()) == NULL)
    {
        ::CloseClipboard();
        return false;
    }
#else // USING_UNICODE
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
#endif // USING_UNICODE
    ::CloseClipboard();
    return true;
}

uint32 WindowsPlatformFunctions::getSetBitCount(const uint8& value)
{
    return uint32(::__popcnt16(uint16(value)));
}

uint32 WindowsPlatformFunctions::getSetBitCount(const uint16& value)
{
    return uint32(::__popcnt16(value));
}

uint32 WindowsPlatformFunctions::getSetBitCount(const uint32& value)
{
    return uint32(::__popcnt(value));
}

uint32 WindowsPlatformFunctions::getSetBitCount(const uint64& value)
{
    return uint32(::__popcnt64(value));
}

bool WindowsPlatformFunctions::wcharToUtf8(std::string& outStr, const WChar* wChar)
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

bool WindowsPlatformFunctions::utf8ToWChar(String& outStr, const AChar* aChar)
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
