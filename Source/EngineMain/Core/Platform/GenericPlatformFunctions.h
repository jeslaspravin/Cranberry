#pragma once
#include "PlatformTypes.h"
#include "../String/String.h"

struct ModuleData
{
    String name;
    String imgName;
    void* basePtr;
    dword moduleSize;
};

template <typename PlatformClass>
class GenericPlatformFunctions
{

protected:


public:

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

    static void getAllModules(void* processHandle, LibPointerPtr* modules,uint32& modulesSize)
    {
        PlatformClass::getAllModules(processHandle, modules, modulesSize);
    }

    static void getModuleInfo(void* processHandle, LibPointer* libraryHandle, ModuleData& moduleData)
    {
        PlatformClass::getModuleInfo(processHandle, libraryHandle, moduleData);
    }
};
