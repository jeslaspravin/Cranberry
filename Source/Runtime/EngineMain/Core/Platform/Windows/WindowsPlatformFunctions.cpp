#include "WindowsPlatformFunctions.h"
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

LibPointer* WindowsPlatformFunctions::openLibrary(String libName)
{
    WindowsLibHandle* handle = new WindowsLibHandle(LoadLibraryA(libName.getChar()),true);

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
    if (handle) {
        FreeLibrary(handle);
    }
}

void* WindowsPlatformFunctions::getProcAddress(const LibPointer* libraryHandle, String symName)
{
    return GetProcAddress(static_cast<const WindowsLibHandle*>(libraryHandle)->libHandle,symName.getChar());
}

void WindowsPlatformFunctions::getModuleInfo(void* processHandle, LibPointer* libraryHandle, ModuleData& moduleData)
{
    if (!libraryHandle)
    {
        return;
    }

    char temp[MAX_PATH];
    MODULEINFO mi;
    HMODULE module = static_cast<WindowsLibHandle*>(libraryHandle)->libHandle;

    GetModuleInformation(processHandle,module,&mi, sizeof(mi));
    moduleData.basePtr = mi.lpBaseOfDll;
    moduleData.moduleSize = mi.SizeOfImage;

    GetModuleFileNameExA(processHandle, module, temp, sizeof(temp));
    moduleData.imgName = temp;
    GetModuleBaseNameA(processHandle, module, temp, sizeof(temp));
    moduleData.name = temp;
}

bool WindowsPlatformFunctions::isSame(const LibPointer* leftHandle, const LibPointer* rightHandle)
{
    return static_cast<const WindowsLibHandle*>(leftHandle)->libHandle == static_cast<const WindowsLibHandle*>
        (rightHandle)->libHandle;
}

void* WindowsPlatformFunctions::getCurrentThreadHandle()
{
    return GetCurrentThread();
}

void* WindowsPlatformFunctions::getCurrentProcessHandle()
{
    return GetCurrentProcess();
}

void WindowsPlatformFunctions::getAllModules(void* processHandle, LibPointerPtr* modules, uint32& modulesSize)
{
    if (modules == nullptr)
    {
        DWORD modSize;
        HMODULE mod;
        EnumProcessModules(processHandle, &mod, 1,&modSize);
        modulesSize = modSize / sizeof(mod);
    }
    else
    {
        std::vector<HMODULE> mods(modulesSize);
        EnumProcessModules(processHandle, mods.data(), modulesSize, (LPDWORD)&modulesSize);
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
    String clipboard;
    if (!::OpenClipboard(NULL))
        return NULL;
    HANDLE clipboardHnd = ::GetClipboardData(CF_UNICODETEXT);
    if (clipboardHnd == NULL)
    {
        ::CloseClipboard();
        return NULL;
    }
    if (const WChar* globalClipboardData = (const WChar*)::GlobalLock(clipboardHnd))
    {
        wcharToStr(clipboard, globalClipboardData);
    }
    ::GlobalUnlock(clipboardHnd);
    ::CloseClipboard();
    return clipboard;
}

bool WindowsPlatformFunctions::setClipboard(const String& text)
{
    if (!::OpenClipboard(NULL))
        return false;
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

void WindowsPlatformFunctions::wcharToStr(String& outStr, const WChar* wChar)
{
    int buf_len = ::WideCharToMultiByte(CP_UTF8, 0, wChar, -1, NULL, 0, NULL, NULL);
    outStr.resize(buf_len);
    ::WideCharToMultiByte(CP_UTF8, 0, wChar, -1, outStr.data(), buf_len, NULL, NULL);
}
