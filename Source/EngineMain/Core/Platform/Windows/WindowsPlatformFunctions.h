#pragma once
#include "../GenericPlatformFunctions.h"

class WindowsPlatformFunctions :public GenericPlatformFunctions<WindowsPlatformFunctions>
{

public:
    static LibPointer* openLibrary(String libName);
    static void releaseLibrary(const LibPointer* libraryHandle);
    static void* getProcAddress(const LibPointer* libraryHandle, String symName);
    static void getModuleInfo(void* processHandle, LibPointer* libraryHandle, ModuleData& moduleData);
    static bool isSame(const LibPointer* leftHandle, const LibPointer* rightHandle);

    static void* getCurrentThreadHandle();
    static void* getCurrentProcessHandle();
    static void getAllModules(void* processHandle, LibPointerPtr* modules, uint32& modulesSize);

    static uint32 getSetBitCount(const uint8& value);
    static uint32 getSetBitCount(const uint16& value);
    static uint32 getSetBitCount(const uint32& value);
    static uint32 getSetBitCount(const uint64& value);
};

namespace GPlatformFunctions {
    typedef GenericPlatformFunctions<WindowsPlatformFunctions> PlatformFunctions;
}