#include "WindowsPlatformFunctions.h"
#include <wtypes.h>
#include <libloaderapi.h>
#include "../ModuleManager.h"

struct WindowsLibHandle : public LibPointer
{
	HMODULE libHandle;
	WindowsLibHandle(HMODULE handle) :libHandle(handle) {}

	~WindowsLibHandle() {
		WindowsPlatformFunctions::releaseLibrary(this);
	}
};


LibPointer* WindowsPlatformFunctions::openLibrary(String libName)
{
	WindowsLibHandle* handle = new WindowsLibHandle(LoadLibraryA(libName.getChar()));

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

