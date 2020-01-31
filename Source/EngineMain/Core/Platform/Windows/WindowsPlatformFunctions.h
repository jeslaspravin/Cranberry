#pragma once
#include "../GenericPlatformFunctions.h"

class WindowsPlatformFunctions :public GenericPlatformFunctions<WindowsPlatformFunctions>
{

public:
	static LibPointer* openLibrary(String libName);
	static void releaseLibrary(const LibPointer* libraryHandle);
	static void* getProcAddress(const LibPointer* libraryHandle, String symName);
};

namespace GPlatformFunctions{
	typedef GenericPlatformFunctions<WindowsPlatformFunctions> PlatformFunctions;
}
