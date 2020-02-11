#pragma once
#include "../GenericAppInstance.h"
#include <windows.h>

struct WindowsAppInstance : public GenericAppInstance {
	HINSTANCE windowsInstance;
	
};



namespace GPlatformInstances {
	typedef WindowsAppInstance PlatformAppInstance;
}