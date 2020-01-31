#pragma once
#include "../GenericAppInstance.h"
#include <windows.h>

struct WindowsAppInstance : public GenericAppInstance {
	HINSTANCE windowsInstance;
	
	HWND getWindowHandle() const;
};



namespace GPlatformInstances {
	typedef WindowsAppInstance PlatformAppInstance;
}