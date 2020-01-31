#pragma once
#include <string>
#include "PlatformTypes.h"
#include "../String/String.h"

template <class PlatformClass>
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
		return PlatformClass::getProcAddress(libraryHandle,symName);
	}
};
